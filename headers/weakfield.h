// weakfield.h
#pragma once

#include "vec3d.h"

typedef struct Simulation Simulation;

typedef struct WeakFieldState
{
  double t;
  Vec3d x;     // position
  Vec3d p;     // covariant spatial momentum components p_i
  double pt;   // p_t (often ~ -E). In quasi-static field we keep it constant.
} WeakFieldState;

void potential_and_gradU(const Simulation *sim, Vec3d x, double *U_out, Vec3d *gradU_out);

WeakFieldState weakfield_rk4_step(WeakFieldState s, const Simulation *sim, double h);

double weakfield_constraint_err(const WeakFieldState *s, const Simulation *sim, bool is_massive);
