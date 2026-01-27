#ifndef SIMULATION_H
#define SIMULATION_H

#include "common.h"

void InitSimulation(SimulationState *sim);
void reset_particle(Particle *p, SystemParams sys, double r0, double phi0, double pr0, bool massive);
void UpdatePhysics(SimulationState *sim);

#endif // SIMULATION_H
