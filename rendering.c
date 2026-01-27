#include "rendering.h"
#include <math.h>
#include <stdio.h>

void DrawScene(const SimulationState *sim) {
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

void DrawUI(const SimulationState *sim) {
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
