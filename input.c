#include "input.h"
#include "simulation.h"
#include <raymath.h>
#include <math.h>

void UpdateCameraControl(Camera3D *camera) {
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

void HandleInput(SimulationState *sim) {
    if (IsKeyPressed(KEY_SPACE))
    {
      sim->paused = !sim->paused;
    }

    // Cycle selection
    if (IsKeyPressed(KEY_TAB))
    {
      int next = IsKeyDown(KEY_LEFT_SHIFT) ? sim->selected_particle_idx - 1 : sim->selected_particle_idx + 1;
      // Handle wrap-around correctly for negative values
      if (next < 0) next = MAX_PARTICLES - 1;
      if (next >= MAX_PARTICLES) next = 0;
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
