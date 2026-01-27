#include "simulation.h"
#include <math.h>

void reset_particle(Particle *p, SystemParams sys, double r0, double phi0, double pr0, bool massive)
{
  p->active = true;
  p->trail_head = 0;
  p->trail_full = false;

  // Clear trail
  for (int i = 0; i < MAX_TRAIL_LENGTH; ++i)
  {
    p->trail[i] = (Vector3) {0};
  }

  p->state.t = 0.0;
  p->state.r = r0;
  p->state.phi = phi0;
  p->state.pr = pr0;

  p->params.is_massive = massive;

  if (massive)
  {
    p->color = BLUE;
    // Calculate circular orbit parameters for stable start
    circular_orbit_constants(r0, sys, &p->params.E, &p->params.L);
  }
  else
  {
    p->color = RED;
    // Photons
    p->params.E = 1.0;
    // b = 5.5
    p->params.L = 5.5 * sys.M * p->params.E;
  }
}

void InitSimulation(SimulationState *sim) {
    sim->sys.G = 1.0;
    sim->sys.M = 1.0;
    sim->sys.c = 1.0;
    sim->r_s = 2.0 * sim->sys.G * sim->sys.M / (sim->sys.c * sim->sys.c);

    // Camera setup
    sim->camera.position = (Vector3) {0.0F, 30.0F, 20.0F};
    sim->camera.target = (Vector3) {0.0F, 0.0F, 0.0F};
    sim->camera.up = (Vector3) {0.0F, 1.0F, 0.0F};
    sim->camera.fovy = 45.0F;
    sim->camera.projection = CAMERA_PERSPECTIVE;

    sim->dt_affine = 0.05;
    sim->steps_per_frame = 10;
    sim->paused = false;
    sim->selected_particle_idx = 0;

    // Initialize particles
    reset_particle(&sim->particles[0], sim->sys, 10.0, 0.0, -0.1, true);
    reset_particle(&sim->particles[1], sim->sys, 20.0, 3.14159, -0.8, false);
}

void UpdatePhysics(SimulationState *sim) {
    if (sim->paused) return;

    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        Particle *p = &sim->particles[i];
        if (!p->active) continue;

        for (int step = 0; step < sim->steps_per_frame; step++)
        {
          if (p->state.r <= sim->r_s * 1.01)
          {
            p->active = false;
            break;
          }
          if (p->state.r > 200.0)
          {
            // Too far (optional handling)
          }

          p->state = rk4_step_geodesic(p->state, sim->sys, p->params, sim->dt_affine);
        }

        // Update Trail
        if (p->active)
        {
          Vector3 pos_curr = {
              (float) (p->state.r * cos(p->state.phi)),
              0.0F,
              (float) (p->state.r * sin(p->state.phi))};

          p->trail[p->trail_head] = pos_curr;
          p->trail_head = (p->trail_head + 1) % MAX_TRAIL_LENGTH;
          if (p->trail_head == 0)
            p->trail_full = true;
        }
    }
}
