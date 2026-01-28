#include "geodesic.h"
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define MAX_TRAIL_LENGTH 1000
#define MAX_PARTICLES    10
#define UI_FONT_SIZE     20

typedef struct
{
  GeodesicState state;
  ParticleParams params;
  Vector3 trail[MAX_TRAIL_LENGTH];
  int trail_head;
  bool trail_full;
  bool active;
  Color color;
} Particle;

typedef struct
{
  SystemParams sys;
  Particle particles[MAX_PARTICLES];
  Camera3D camera;
  int selected_particle_idx;
  bool paused;
  double dt_affine;
  int steps_per_frame;
  double r_s;   // Schwarzschild radius
} SimulationState;

// Helper to reset a single particle
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

void InitSimulation(SimulationState *sim)
{
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

void HandleInput(SimulationState *sim)
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
    float change_speed = 0.05F;
    if (IsKeyDown(KEY_LEFT_SHIFT))
      change_speed = 0.005F;

    // Change Radial Momentum (pr)
    if (IsKeyPressed(KEY_Q) || IsKeyDown(KEY_Q))
      sel_p->state.pr -= change_speed;
    if (IsKeyPressed(KEY_W) || IsKeyDown(KEY_W))
      sel_p->state.pr += change_speed;

    // Change Angular Momentum (L)
    if (IsKeyPressed(KEY_A) || IsKeyDown(KEY_A))
      sel_p->params.L -= change_speed;
    if (IsKeyPressed(KEY_S) || IsKeyDown(KEY_S))
      sel_p->params.L += change_speed;
  }

  // Reset Everything
  if (IsKeyPressed(KEY_R))
  {
    reset_particle(&sim->particles[0], sim->sys, 10.0, 0.0, -0.1, true);
    reset_particle(&sim->particles[1], sim->sys, 20.0, 3.14159, -0.8, false);
    // Reset others if we added more
    sim->selected_particle_idx = 0;
  }
}

void UpdatePhysics(SimulationState *sim)
{
  if (sim->paused)
    return;

  for (int i = 0; i < MAX_PARTICLES; i++)
  {
    Particle *p = &sim->particles[i];
    if (!p->active)
      continue;

    for (int step = 0; step < sim->steps_per_frame; step++)
    {
      if (p->state.r <= sim->r_s * 1.01)
      {
        p->active = false;
        break;
      }
      if (p->state.r > 200.0)
      {
        // Too far
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

void DrawScene(const SimulationState *sim)
{
  BeginMode3D(sim->camera);
  DrawGrid(20, 1.0F);

  // Draw Axes
  DrawLine3D((Vector3) {0, 0, 0}, (Vector3) {5, 0, 0}, RED);
  DrawLine3D((Vector3) {0, 0, 0}, (Vector3) {0, 5, 0}, GREEN);
  DrawLine3D((Vector3) {0, 0, 0}, (Vector3) {0, 0, 5}, BLUE);

  // Draw Black Hole
  DrawSphere((Vector3) {0, 0, 0}, (float) sim->r_s, BLACK);

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
      Vector3 pos = {
          (float) (p->state.r * cos(p->state.phi)),
          0.0F,
          (float) (p->state.r * sin(p->state.phi))};

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

void DrawUI(const SimulationState *sim)
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

  char buffer[128];
  snprintf(buffer, sizeof(buffer), "Selected Particle: %d (%s)",
           sim->selected_particle_idx,
           sel_p->params.is_massive ? "Massive" : "Photon");
  DrawText(buffer, 20, y + 10, UI_FONT_SIZE, BLACK);

  if (sel_p->active)
  {
    snprintf(buffer, sizeof(buffer), "Radius (r): %.3f", sel_p->state.r);
    DrawText(buffer, 20, y + 40, UI_FONT_SIZE, BLACK);

    snprintf(buffer, sizeof(buffer), "Phi (deg): %.1f", sel_p->state.phi * 180.0 / PI);
    DrawText(buffer, 20, y + 65, UI_FONT_SIZE, BLACK);

    snprintf(buffer, sizeof(buffer), "Radial Mom (pr): %.3f  [Q/W]", sel_p->state.pr);
    DrawText(buffer, 20, y + 90, UI_FONT_SIZE, DARKBLUE);

    snprintf(buffer, sizeof(buffer), "Ang Mom (L): %.3f    [A/S]", sel_p->params.L);
    DrawText(buffer, 20, y + 115, UI_FONT_SIZE, DARKBLUE);

    snprintf(buffer, sizeof(buffer), "Energy (E): %.3f", sel_p->params.E);
    DrawText(buffer, 20, y + 140, UI_FONT_SIZE, BLACK);
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

  SimulationState sim = {0};
  InitSimulation(&sim);

  // Main game loop
  while (!WindowShouldClose())
  {
    UpdateCameraControl(&sim.camera);
    HandleInput(&sim);
    UpdatePhysics(&sim);

    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawScene(&sim);
    DrawUI(&sim);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
