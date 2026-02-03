#include "body.h"
#include "simulation.h"
#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "clay_renderer_raylib.c"
#include <math.h>
#include <raylib.h>
#include <raymath.h>

#define FONT_DEFAULT_PATH "resources/DejaVuSans.ttf"
#define UI_FONT_SIZE      22

Font fonts[1];

Clay_String Clay_String_FromChar(const char *chars)
{
  return (Clay_String) {.length = (int32_t) strlen(chars), .chars = chars};
}

void update_camera_control(Camera3D *camera)
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

void handle_input(Simulation *sim, float frame_dt)
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
      TraceLog(LOG_DEBUG, "Pressed Q");
      if (sel_p->model == PARTICLE_MODEL_SCHW)
        sel_p->state.pr -= base;
      else
      {
        // Add -base along r-hat in xz plane
        Vec3d x = sel_p->wf.x;
        double phi = atan2(x.z, x.x);
        Vec3d rhat = V3d(cos(phi), 0.0, sin(phi));
        sel_p->wf.p = v3d_add(sel_p->wf.p, v3d_scale(rhat, -base));
        SystemParams sys = {sim->G, sim->bodies[0].mass, sim->c};
        enforce_constraint_schwarzschild(sel_p, sys);
      }
    }

    if (IsKeyDown(KEY_W))
    {
      TraceLog(LOG_DEBUG, "Pressed W");
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
      init_particle(&sim->particles[0], sys, 10.0, 0.0, -0.1, true);
      init_particle(&sim->particles[1], sys, 20.0, 3.14159, -0.8, false);
      // reset others to inactive
      for (int i = 2; i < MAX_PARTICLES; i++)
        sim->particles[i].active = false;
    }
    sim->selected_particle_idx = 0;
  }
}

void draw_scene(const Simulation *sim)
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

Clay_TextElementConfig ui_text_config = {.fontSize = UI_FONT_SIZE, .textColor = {0, 0, 0, 255}};

void RenderSidebar(const Simulation *sim)
{
  CLAY(CLAY_ID("Sidebar"), {.layout = {.layoutDirection = CLAY_TOP_TO_BOTTOM,
                                       .padding = {16, 16, 16, 16},
                                       .childGap = 8,
                                       .sizing = {.width = CLAY_SIZING_FIXED(350)}},
                            .backgroundColor = {200, 200, 200, 150},
                            .cornerRadius = {10, 10, 10, 10}})
  {
    const Particle *sel_p = &sim->particles[sim->selected_particle_idx];

    CLAY_TEXT(
        Clay_String_FromChar(
            TextFormat("Selected Particle: %d (%s)", sim->selected_particle_idx, sel_p->params.is_massive ? "Massive" : "Photon")),
        Clay__StoreTextElementConfig(ui_text_config));

    if (sel_p->active)
    {
      CLAY_TEXT(
          Clay_String_FromChar(TextFormat("Radius (r): %.3f", sel_p->state.r)),
          Clay__StoreTextElementConfig(ui_text_config));
      CLAY_TEXT(Clay_String_FromChar(TextFormat("Phi (deg): %.1f", sel_p->state.phi * 180.0 / PI)),
                Clay__StoreTextElementConfig(ui_text_config));

      Clay_TextElementConfig mom_config = ui_text_config;

      mom_config.textColor = (Clay_Color) {0, 0, 255, 255};

      CLAY_TEXT(Clay_String_FromChar(TextFormat("Radial Mom (pr): %.3f", sel_p->state.pr)),
                CLAY_TEXT_CONFIG(mom_config));

      CLAY_TEXT(Clay_String_FromChar(TextFormat("Energy (E): %.3f", sel_p->params.E)),
                Clay__StoreTextElementConfig(ui_text_config));

      CLAY_TEXT(Clay_String_FromChar(TextFormat("Model: %s", sel_p->model == PARTICLE_MODEL_SCHW ? "Schwarzschild" : "Weak-field")),
                Clay__StoreTextElementConfig(ui_text_config));

      Clay_TextElementConfig err_config = ui_text_config;
      err_config.textColor = (Clay_Color) {200, 0, 0, 255};
      CLAY_TEXT(Clay_String_FromChar(TextFormat("Constraint err: %.3e", sel_p->constraint_err)),
                CLAY_TEXT_CONFIG(err_config));
    }
    else
    {
      Clay_TextElementConfig inactive_config = ui_text_config;
      inactive_config.textColor = (Clay_Color) {255, 0, 0, 255};
      CLAY_TEXT(CLAY_STRING("INACTIVE (Fell in or not init)"),
                CLAY_TEXT_CONFIG(inactive_config));
    }
  }
}

void RenderTopInstructions()
{
  CLAY(CLAY_ID("TopInfo"), {.layout = {.sizing = {.width = CLAY_SIZING_GROW()}}})
  {
    CLAY(CLAY_ID("InstructionList"), {.layout = {.layoutDirection = CLAY_TOP_TO_BOTTOM, .childGap = 4}})
    {
      Clay_TextElementConfig instr_config = ui_text_config;
      instr_config.textColor = (Clay_Color) {80, 80, 80, 255};
      CLAY_TEXT(CLAY_STRING("Camera: Left drag to Orbit, Wheel to Zoom"), CLAY_TEXT_CONFIG(instr_config));
      CLAY_TEXT(CLAY_STRING("[TAB] Select Particle | [SPACE] Pause | [R] Reset All"), CLAY_TEXT_CONFIG(instr_config));
    }
  }
}

void draw_ui(const Simulation *sim)
{
  DrawFPS(GetScreenWidth() - 100, 10);
  Clay_BeginLayout();

  CLAY(CLAY_ID("MainContainer"), {.layout = {.sizing = {.width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_GROW()},
                                             .padding = {16, 16, 16, 16}}})
  {
    RenderSidebar(sim);
    RenderTopInstructions();
  }

  Clay_RenderCommandArray renderCommands = Clay_EndLayout();
  Clay_Raylib_Render(renderCommands, fonts);
}

int main(void)
{
  // Initialization
  const int screenWidth = 1280;
  const int screenHeight = 720;

  SetTraceLogLevel(LOG_DEBUG);
  InitWindow(screenWidth, screenHeight, "Schwarzschild Geodesic - Spg");
  SetTargetFPS(60);

  // Initialize Clay
  uint64_t clayMemorySize = Clay_MinMemorySize();
  Clay_Arena clayMemory = Clay_CreateArenaWithCapacityAndMemory(clayMemorySize, malloc(clayMemorySize));
  Clay_Initialize(clayMemory, (Clay_Dimensions) {(float) screenWidth, (float) screenHeight}, (Clay_ErrorHandler) {0});
  fonts[0] = LoadFont(FONT_DEFAULT_PATH);
  Clay_SetMeasureTextFunction(Raylib_MeasureText, fonts);

  Simulation sim = {0};
  init_simulation(&sim);

  // Initialize Body 0
  double M = 1.0;
  double rs = 2.0 * sim.G * M / (sim.c * sim.c);
  init_body(&sim.bodies[0], (Vector3) {5, 0, 0}, M, rs, BLACK);
  // Velocity for circular orbit: V = sqrt(G*M / (4*R)) = sqrt(1/(20)) approx 0.2236
  sim.bodies[0].velocity = (Vector3) {0, 0, 0.223607F};
  sim.body_count = 1;

  // Body 1
  init_body(&sim.bodies[sim.body_count], (Vector3) {-5, 0, 0}, 1.0, rs, BLACK);
  sim.bodies[sim.body_count].velocity = (Vector3) {0, 0, -0.223607F};
  sim.body_count++;

  // We construct a temporary SystemParams.
  SystemParams sys = {sim.G, sim.bodies[0].mass, sim.c};

  // Particle 0: Massive, far out
  init_particle(&sim.particles[0], sys, 15.0, 0.0, 0.0, true);

  // Particle 1: Photon
  init_particle(&sim.particles[1], sys, 20.0, 3.14159, -0.8, false);

  sim.particle_count = 2;

  // Main game loop
  while (!WindowShouldClose())
  {
    update_camera_control(&sim.camera);
    float frame_dt = GetFrameTime();
    handle_input(&sim, frame_dt);
    update_simulation_physics(&sim, frame_dt);

    // Update Clay State
    Clay_SetLayoutDimensions((Clay_Dimensions) {(float) GetScreenWidth(), (float) GetScreenHeight()});
    Vector2 mousePos = GetMousePosition();
    Clay_SetPointerState((Clay_Vector2) {mousePos.x, mousePos.y}, IsMouseButtonDown(MOUSE_BUTTON_LEFT));
    Clay_UpdateScrollContainers(true, (Clay_Vector2) {0, GetMouseWheelMove()}, frame_dt);

    BeginDrawing();
    ClearBackground(RAYWHITE);
    draw_scene(&sim);
    draw_ui(&sim);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
