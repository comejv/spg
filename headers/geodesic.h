#pragma once

#include <stdbool.h>
#include "vec3d.h"

// State of the particle in Schwarzschild coordinates (equatorial plane)
// We treat theta = pi/2 fixed, so we only track t, r, phi.
typedef struct
{
  double t;     // Coordinate time
  union {
    Vec3d u;    // Vector container for (r, phi, pr)
    struct {
      double r;     // Radial coordinate
      double phi;   // Azimuthal angle
      double pr;    // Radial momentum
    };
  };
} GeodesicState;

// Physical parameters for the central object (System)
typedef struct
{
  double G;   // Gravitational constant
  double M;   // Mass of the central object
  double c;   // Speed of light
} SystemParams;

// Forward declaration of ParticleParams (defined in particle.h)
typedef struct ParticleParams ParticleParams;

/**
 * @brief Computes the derivative of the state vector (dt, dr, dphi, dpr).
 *
 * @param state Current state
 * @param sys System parameters (G, M, c)
 * @param p Particle parameters (E, L, is_massive)
 * @return Derivative of the state
 */
GeodesicState geodesic_deriv(GeodesicState state, SystemParams sys, const ParticleParams *p);

/**
 * @brief Performs a single RK4 integration step.
 *
 * @param state Current state
 * @param sys System parameters
 * @param p Particle parameters
 * @param step_size Step size in affine parameter (lambda/tau)
 * @return New state after the step
 */
GeodesicState rk4_step_geodesic(GeodesicState s, SystemParams sys, const ParticleParams *p, double h);

/**
 * @brief Calculates the constants E and L for a circular orbit.
 *
 * @param r0 Initial radius
 * @param sys System parameters
 * @param E_out Output pointer for Energy
 * @param L_out Output pointer for Angular Momentum
 */
void circular_orbit_constants(double r0, SystemParams sys,
                              double *E_out, double *L_out);
