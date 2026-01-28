#include "simulation.h"
#include <math.h>
#include <raymath.h>

void InitSimulation(Simulation *sim)
{
  sim->G = 1.0;
  sim->c = 1.0;

  // Camera setup
  sim->camera.position = (Vector3) {0.0F, 30.0F, 20.0F};
  sim->camera.target = (Vector3) {0.0F, 0.0F, 0.0F};
  sim->camera.up = (Vector3) {0.0F, 1.0F, 0.0F};
  sim->camera.fovy = 45.0F;
  sim->camera.projection = CAMERA_PERSPECTIVE;

  sim->selected_particle_idx = 0;

  sim->paused = false;
  sim->fixed_dt = 0.01;
  sim->time_accum = 0.0;
  sim->sim_speed = 1.0;
  sim->max_substeps = 200;
}

#include "weakfield.h"

static inline double clampd(double x, double a, double b)
{
  if (x < a)
    return a;
  if (x > b)
    return b;
  return x;
}

Vector3 particle_world_pos(const Particle *p)
{
  if (p->model == PARTICLE_MODEL_WEAKFIELD)
    return v3d_to_v3(p->wf.x);

  // Schwarzschild equatorial rendering
  return (Vector3) {
      (float) (p->state.r * cos(p->state.phi)),
      0.0F,
      (float) (p->state.r * sin(p->state.phi))};
}

static double schw_constraint_err(const Particle *p, SystemParams sys)
{
  double r = p->state.r;
  double pr = p->state.pr;
  double E = p->params.E;
  double L = p->params.L;

  double rs = 2.0 * sys.G * sys.M / (sys.c * sys.c);
  double f = 1.0 - (rs / r);
  if (f <= 1e-12)
    f = 1e-12;

  // g^tt = -1/(f c^2), g^rr = f, g^pp = 1/r^2
  double gtt = -1.0 / (f * sys.c * sys.c);
  double grr = f;
  double gpp = 1.0 / (r * r);

  double twoH = (gtt * (E * E)) + (grr * (pr * pr)) + (gpp * (L * L));

  double target = p->params.is_massive ? (-(sys.c * sys.c)) : 0.0;
  return twoH - target;
}

static void UpdateBodies(Simulation *sim, double dt)
{
  // Simple N-body Newtonian gravity (Semi-implicit Euler)
  // Calculate accelerations
  Vector3 acc[MAX_BODIES] = {0};

  for (int i = 0; i < sim->body_count; i++)
  {
    for (int j = 0; j < sim->body_count; j++)
    {
      if (i == j)
        continue;

      Vector3 r_vec = Vector3Subtract(sim->bodies[j].position, sim->bodies[i].position);
      float dist2 = Vector3LengthSqr(r_vec);
      float dist = sqrtf(dist2);

      // Softening
      if (dist < 0.1F)
        dist = 0.1F;

      // F = G * m1 * m2 / r^2
      // a = F / m1 = G * m2 / r^2
      // vec_a = a * (r_vec / r) = G * m2 * r_vec / r^3

      float scale = (float) (sim->G * sim->bodies[j].mass / (dist * dist * dist));
      acc[i] = Vector3Add(acc[i], Vector3Scale(r_vec, scale));
    }
  }

  // Update State
  for (int i = 0; i < sim->body_count; i++)
  {
    // v += a * dt
    sim->bodies[i].velocity = Vector3Add(sim->bodies[i].velocity, Vector3Scale(acc[i], (float) dt));

    // x += v * dt
    sim->bodies[i].position = Vector3Add(sim->bodies[i].position, Vector3Scale(sim->bodies[i].velocity, (float) dt));
  }
}

void UpdateSimulationPhysics(Simulation *sim, float frame_dt)
{
  if (sim->paused)
    return;
  if (sim->body_count == 0)
    return;

  sim->time_accum += (double) frame_dt * sim->sim_speed;

  int substeps = 0;
  while (sim->time_accum >= sim->fixed_dt && substeps < sim->max_substeps)
  {
    // Update Bodies first
    UpdateBodies(sim, sim->fixed_dt);

    bool use_schw = (sim->body_count == 1);

    Body *b0 = &sim->bodies[0];
    SystemParams sys0 = {sim->G, b0->mass, sim->c};
    double rs0 = 2.0 * sim->G * b0->mass / (sim->c * sim->c);

    for (int i = 0; i < MAX_PARTICLES; i++)
    {
      Particle *p = &sim->particles[i];
      if (!p->active)
        continue;

      // AUTO switch
      if (use_schw && p->model == PARTICLE_MODEL_WEAKFIELD)
        particle_weakfield_to_schw(p, sys0);
      if (!use_schw && p->model == PARTICLE_MODEL_SCHW)
        particle_schw_to_weakfield(p, sim);

      if (p->model == PARTICLE_MODEL_SCHW)
      {
        // Horizon stop
        if (p->state.r <= rs0 * 1.01)
        {
          p->active = false;
          continue;
        }

        // Mild step shrinking near horizon (helps stability)
        double f = 1.0 - (rs0 / p->state.r);
        double h = sim->fixed_dt * clampd(f, 0.05, 1.0);

        p->state = rk4_step_geodesic(p->state, sys0, &p->params, h);
        p->constraint_err = schw_constraint_err(p, sys0);
      }
      else
      {
        // Deactivate if inside any body radius (simple collision)
        for (int b = 0; b < sim->body_count; b++)
        {
          Vec3d xb = v3d_from_v3(sim->bodies[b].position);
          Vec3d d = v3d_sub(p->wf.x, xb);
          double r = v3d_len(d);
          if (r <= sim->bodies[b].radius * 1.01)
          {
            p->active = false;
            break;
          }
        }
        if (!p->active)
          continue;

        p->wf = weakfield_rk4_step(p->wf, sim, sim->fixed_dt);
        p->constraint_err = weakfield_constraint_err(
            &p->wf, sim, p->params.is_massive);
      }

      // Trail update (world coordinates)
      Vector3 pos = particle_world_pos(p);
      p->trail[p->trail_head] = pos;
      p->trail_head = (p->trail_head + 1) % MAX_TRAIL_LENGTH;
      if (p->trail_head == 0)
        p->trail_full = true;
    }

    sim->time_accum -= sim->fixed_dt;
    substeps++;
  }

  if (substeps >= sim->max_substeps)
    sim->time_accum = 0.0;
}
