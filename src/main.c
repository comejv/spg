#include "body.h"
#include "simulation.h"
#include <math.h>
#include <raylib.h>
#include <raymath.h>

#define UI_FONT_SIZE 20

void UpdateCameraControl(Camera3D *camera)
{
  Vector3 d = Vector3Subtract(camera->position, camera->target);
  float dist = Vector3Length(d);

  // Calculate current angles
  float camera_yaw = atan2f(d.x, d.z);
  float camera_pitch = atan2f(d.y, sqrtf((d.x * d.x) + (d.z * d.z)));

  // Mouse Input
  if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
  {
    Vector2 delta = GetMouseDelta();
    camera_yaw -= delta.x * 0.01F;
    camera_pitch -= delta.y * 0.01F;
  }

  // Clamp pitch to avoid gimbal lock/flipping
  if (camera_pitch > 1.5F)
    camera_pitch = 1.5F;
  if (camera_pitch < -1.5F)
    camera_pitch = -1.5F;

  // Zoom
  float wheel = GetMouseWheelMove();
  if (wheel != 0)
  {
    dist -= wheel * 2.0F;
    if (dist < 2.0F)
      dist = 2.0F;
  }

  // Reconstruct position
  camera->position.x = camera->target.x + dist * cosf(camera_pitch) * sinf(camera_yaw);
  camera->position.z = camera->target.z + dist * cosf(camera_pitch) * cosf(camera_yaw);
  camera->position.y = camera->target.y + dist * sinf(camera_pitch);
}

void HandleInput(Simulation *sim, float frame_dt)
{
  if (IsKeyPressed(KEY_SPACE))
  {
    sim->paused = !sim->paused;
  }

  // Cycle selection
  if (IsKeyPressed(KEY_TAB))
  {
    int next = IsKeyDown(KEY_LEFT_SHIFT) ? sim->selected_particle_idx - 1 : sim->selected_particle_idx + 1;
    // Handle wrap-around correctly for negative values
    if (next < 0)
      next = MAX_PARTICLES - 1;
    if (next >= MAX_PARTICLES)
      next = 0;
    sim->selected_particle_idx = next;
  }

  // Modify Selected Particle
  Particle *sel_p = &sim->particles[sim->selected_particle_idx];
  if (sel_p->active)
  {
    double base = 0.8 * (double) frame_dt;
    if (IsKeyDown(KEY_LEFT_SHIFT))
      base *= 0.1;

    // Radial momentum
    if (IsKeyDown(KEY_Q))
    {
      if (sel_p->model == PARTICLE_MODEL_SCHW)
        sel_p->state.pr -= base;
      else
      {
        // Add -base along r-hat in xz plane
        Vec3d x = sel_p->wf.x;
        double phi = atan2(x.z, x.x);
        Vec3d rhat = V3d(cos(phi), 0.0, sin(phi));
        sel_p->wf.p = v3d_add(sel_p->wf.p, v3d_scale(rhat, -base));
      }
    }

    if (IsKeyDown(KEY_W))
    {
      if (sel_p->model == PARTICLE_MODEL_SCHW)
        sel_p->state.pr += base;
      else
      {
        Vec3d x = sel_p->wf.x;
        double phi = atan2(x.z, x.x);
        Vec3d rhat = V3d(cos(phi), 0.0, sin(phi));
        sel_p->wf.p = v3d_add(sel_p->wf.p, v3d_scale(rhat, +base));
      }
    }
  }

  // Change Simulation Speed
  if (IsKeyDown(KEY_P))
  {
    sim->sim_speed *= 1.25;
  }
  else if (IsKeyDown(KEY_M))
  {
    sim->sim_speed /= 1.25;
  }

  // Reset Everything
  if (IsKeyPressed(KEY_R))
  {
    // Re-init particles 0 and 1
    if (sim->body_count > 0)
    {
      SystemParams sys = {sim->G, sim->bodies[0].mass, sim->c};
      InitParticle(&sim->particles[0], sys, 10.0, 0.0, -0.1, true);
      InitParticle(&sim->particles[1], sys, 20.0, 3.14159, -0.8, false);
      // reset others to inactive?
      for (int i = 2; i < MAX_PARTICLES; i++)
        sim->particles[i].active = false;
    }
    sim->selected_particle_idx = 0;
  }
}

void DrawScene(const Simulation *sim)
{
  BeginMode3D(sim->camera);
  DrawGrid(20, 1.0F);

  // Draw Axes
  DrawLine3D((Vector3) {0, 0, 0}, (Vector3) {5, 0, 0}, RED);
  DrawLine3D((Vector3) {0, 0, 0}, (Vector3) {0, 5, 0}, GREEN);
  DrawLine3D((Vector3) {0, 0, 0}, (Vector3) {0, 0, 5}, BLUE);

  // Draw Bodies
  for (int i = 0; i < sim->body_count; i++)
  {
    DrawSphere(sim->bodies[i].position, (float) sim->bodies[i].radius, sim->bodies[i].color);
  }

  // Draw Particles
  for (int i = 0; i < MAX_PARTICLES; i++)
  {
    const Particle *p = &sim->particles[i];
    if (p->trail_head == 0 && !p->trail_full && !p->active)
      continue;

    // Draw trail
    int count = p->trail_full ? MAX_TRAIL_LENGTH : p->trail_head;
    int start = p->trail_full ? p->trail_head : 0;
    for (int j = 0; j < count - 1; j++)
    {
      int idx1 = (start + j) % MAX_TRAIL_LENGTH;
      int idx2 = (start + j + 1) % MAX_TRAIL_LENGTH;
      // Highlight selected particle trail
      Color t_col = p->color;
      if (i == sim->selected_particle_idx)
      {
        t_col = PURPLE;
      }
      DrawLine3D(p->trail[idx1], p->trail[idx2], Fade(t_col, 0.6F));
    }

    if (p->active)
    {
      Vector3 pos = particle_world_pos(p);

      Color p_col = p->color;
      float size = 0.2F;
      if (i == sim->selected_particle_idx)
      {
        p_col = GOLD;   // Highlight selected
        size = 0.3F;
      }
      DrawSphere(pos, size, p_col);
    }
  }
  EndMode3D();
}

void DrawUI(const Simulation *sim)
{
  DrawText("Schwarzschild Geodesic", 10, 10, UI_FONT_SIZE, DARKGRAY);

  int y = 40;
  DrawText("Camera: Left drag to Orbit, Wheel to Zoom", 10, y, UI_FONT_SIZE, DARKGRAY);
  y += 25;
  DrawText("[TAB] Select Particle | [SPACE] Pause | [R] Reset All", 10, y, UI_FONT_SIZE, DARKGRAY);
  y += 30;

  // Info Panel
  DrawRectangle(10, y, 350, 200, Fade(LIGHTGRAY, 0.5F));
  DrawRectangleLines(10, y, 350, 200, GRAY);

  const Particle *sel_p = &sim->particles[sim->selected_particle_idx];

  DrawText(TextFormat("Selected Particle: %d (%s)", sim->selected_particle_idx, sel_p->params.is_massive ? "Massive" : "Photon"),
           20, y + 10, UI_FONT_SIZE, BLACK);

  if (sel_p->active)
  {
    DrawText(TextFormat("Radius (r): %.3f", sel_p->state.r), 20, y + 40, UI_FONT_SIZE, BLACK);
    DrawText(TextFormat("Phi (deg): %.1f", sel_p->state.phi * 180.0 / PI), 20, y + 65, UI_FONT_SIZE, BLACK);
    DrawText(TextFormat("Radial Mom (pr): %.3f  [Q/W]", sel_p->state.pr), 20, y + 90, UI_FONT_SIZE, DARKBLUE);
    DrawText(TextFormat("Model: %s", sel_p->model == PARTICLE_MODEL_SCHW ? "Schwarzschild" : "Weak-field multi-body"),
             20, y + 165, UI_FONT_SIZE, BLACK);

    DrawText(TextFormat("Constraint err: %.3e", sel_p->constraint_err), 20, y + 190, UI_FONT_SIZE, MAROON);
    DrawText(TextFormat("Energy (E): %.3f", sel_p->params.E), 20, y + 140, UI_FONT_SIZE, BLACK);
  }
  else
  {
    DrawText("INACTIVE (Fell in or not init)", 20, y + 50, UI_FONT_SIZE, RED);
  }
}

int main(void)
{
  // Initialization
  const int screenWidth = 1280;
  const int screenHeight = 720;

  InitWindow(screenWidth, screenHeight, "Schwarzschild Geodesic - Spg");
  SetTargetFPS(60);

  Simulation sim = {0};
  InitSimulation(&sim);

  // Initialize Body 0 (Binary Component 1)
  double M = 1.0;
  double rs = 2.0 * sim.G * M / (sim.c * sim.c);
  InitBody(&sim.bodies[0], (Vector3) {5, 0, 0}, M, rs, BLACK);
  // Velocity for circular orbit: V = sqrt(G*M / (4*R)) = sqrt(1/(20)) approx 0.2236
  sim.bodies[0].velocity = (Vector3) {0, 0, 0.223607F};
  sim.body_count = 1;

  // Body 1 (Binary Component 2)
  InitBody(&sim.bodies[sim.body_count], (Vector3) {-5, 0, 0}, 1.0, rs, BLACK);
  sim.bodies[sim.body_count].velocity = (Vector3) {0, 0, -0.223607F};
  sim.body_count++;

  // We construct a temporary SystemParams.
  // Note: InitParticle calculates orbit for single body mass M.
  // Since we have 2M total, these particles will be in elliptical orbits or unbound unless we tweak.
  SystemParams sys = {sim.G, sim.bodies[0].mass, sim.c};

  // Particle 0: Massive, far out
  InitParticle(&sim.particles[0], sys, 15.0, 0.0, 0.0, true);
  // Manually boost L to account for higher central mass (approx sqrt(2) * L_circ_single)
  sim.particles[0].params.L *= 1.414;

  // Particle 1: Photon
  InitParticle(&sim.particles[1], sys, 20.0, 3.14159, -0.8, false);

  sim.particle_count = 2;

  // Main game loop
  while (!WindowShouldClose())
  {
    UpdateCameraControl(&sim.camera);
    float frame_dt = GetFrameTime();
    HandleInput(&sim, frame_dt);
    UpdateSimulationPhysics(&sim, frame_dt);

    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawScene(&sim);
    DrawUI(&sim);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
