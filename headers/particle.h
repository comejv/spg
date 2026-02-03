#pragma once

#include "geodesic.h"
#include "weakfield.h"
#include <raylib.h>

#define MAX_TRAIL_LENGTH 100

// Physical parameters for the particle
typedef struct ParticleParams
{
  double E;          // Energy per unit mass (or Energy parameter for photons)
  double L;          // Angular momentum per unit mass
  bool is_massive;   // true for massive particles (H = -c^2/2), false for photons (H = 0)
} ParticleParams;

typedef enum ParticleModel
{
  PARTICLE_MODEL_SCHW = 0,
  PARTICLE_MODEL_WEAKFIELD = 1
} ParticleModel;

typedef struct
{
  ParticleModel model;
  WeakFieldState wf;
  double constraint_err;
  GeodesicState state;
  ParticleParams params;
  Vector3 trail[MAX_TRAIL_LENGTH];
  int trail_head;
  bool trail_full;
  bool active;
  Color color;
} Particle;

static inline double mu_geom(SystemParams sys)
{
  return sys.G * sys.M / (sys.c * sys.c);
}

static inline double rs_schw(SystemParams sys)
{
  return 2.0 * mu_geom(sys);
}

// Initializes a particle with given parameters
void init_particle(Particle *p, SystemParams sys, double r0, double phi0, double pr0, bool massive);

struct Simulation;
void particle_schw_to_weakfield(Particle *p, const struct Simulation *sim);
void particle_weakfield_to_schw(Particle *p, SystemParams sys);

Vector3 particle_world_pos(const Particle *p);

void enforce_constraint_schwarzschild(Particle *p, SystemParams sys);
