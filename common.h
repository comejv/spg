#ifndef COMMON_H
#define COMMON_H

#include "geodesic.h"
#include <raylib.h>
#include <stdbool.h>

#define MAX_TRAIL_LENGTH 1000
#define MAX_PARTICLES    10
#define UI_FONT_SIZE     20

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

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

#endif   // COMMON_H
