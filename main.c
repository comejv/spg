#include "input.h"
#include "rendering.h"
#include "simulation.h"
#include <raylib.h>

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
