#pragma once

#include "body.h"
#include "particle.h"
#include <raylib.h>

#define MAX_PARTICLES 10
#define MAX_BODIES    5

typedef struct Simulation
{
  // Global Constants
  double G;
  double c;

  // Entities
  Body bodies[MAX_BODIES];
  int body_count;

  Particle particles[MAX_PARTICLES];
  int particle_count;

  // Simulation Control
  bool paused;
  double dt_affine;
  int steps_per_frame;

  // UI / Interaction State
  Camera3D camera;
  int selected_particle_idx;

  // Sim time
  double fixed_dt;     // integration step in affine parameter
  double time_accum;   // accumulator (scaled by sim_speed)
  double sim_speed;    // 1.0 = realtime-ish stepping
  int max_substeps;    // safety cap

} Simulation;

void InitSimulation(Simulation *sim);
void UpdateSimulationPhysics(Simulation *sim, float frame_dt);
