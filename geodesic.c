#include "geodesic.h"
#include <math.h>

GeodesicState geodesic_deriv(GeodesicState state, SystemParams sys, ParticleParams p)
{
  GeodesicState d_state = {0};

  double r = state.r;
  double pr = state.pr;
  double G = sys.G;
  double M = sys.M;
  double c = sys.c;
  double E = p.E;
  double L = p.L;

  double rs = 2.0 * G * M / (c * c);
  double f = 1.0 - (rs / r);

  // Safety check for horizon
  if (f <= 1e-9)
  {
    return d_state;
  }

  double d_gtt_dr = rs / (r * r * c * c * f * f);

  double d_grr_dr = rs / (r * r);

  double d_gpp_dr = -2.0 / (r * r * r);

  // 1. Position derivatives: x_dot = g^uv * p_v
  // dt/dlambda = g^tt * p_t = (-1/(f c^2)) * (-E) = E / (f c^2)
  d_state.t = E / (f * c * c);

  d_state.r = f * pr;

  // dphi/dlambda = g^phiphi * p_phi = (1/r^2) * L
  d_state.phi = L / (r * r);

  // 2. Momentum derivative: pr_dot = -dH/dr
  // H = 1/2 * (g^tt pt^2 + g^rr pr^2 + g^pp p_phi^2)
  // dH/dr = 1/2 * ( (d_gtt)*(-E)^2 + (d_grr)*(pr)^2 + (d_gpp)*(L)^2 )

  double term1 = d_gtt_dr * E * E;
  double term2 = d_grr_dr * pr * pr;
  double term3 = d_gpp_dr * L * L;

  d_state.pr = -0.5 * (term1 + term2 + term3);

  return d_state;
}

GeodesicState rk4_step_geodesic(GeodesicState s, SystemParams sys, ParticleParams p, double h)
{
  // k1
  GeodesicState k1 = geodesic_deriv(s, sys, p);

  // k2
  GeodesicState s2;
  s2.t = s.t + 0.5 * h * k1.t;
  s2.r = s.r + 0.5 * h * k1.r;
  s2.phi = s.phi + 0.5 * h * k1.phi;
  s2.pr = s.pr + 0.5 * h * k1.pr;
  GeodesicState k2 = geodesic_deriv(s2, sys, p);

  // k3
  GeodesicState s3;
  s3.t = s.t + 0.5 * h * k2.t;
  s3.r = s.r + 0.5 * h * k2.r;
  s3.phi = s.phi + 0.5 * h * k2.phi;
  s3.pr = s.pr + 0.5 * h * k2.pr;
  GeodesicState k3 = geodesic_deriv(s3, sys, p);

  // k4
  GeodesicState s4;
  s4.t = s.t + h * k3.t;
  s4.r = s.r + h * k3.r;
  s4.phi = s.phi + h * k3.phi;
  s4.pr = s.pr + h * k3.pr;
  GeodesicState k4 = geodesic_deriv(s4, sys, p);

  // Combine
  GeodesicState result;
  result.t = s.t + (h / 6.0) * (k1.t + 2 * k2.t + 2 * k3.t + k4.t);
  result.r = s.r + (h / 6.0) * (k1.r + 2 * k2.r + 2 * k3.r + k4.r);
  result.phi = s.phi + (h / 6.0) * (k1.phi + 2 * k2.phi + 2 * k3.phi + k4.phi);
  result.pr = s.pr + (h / 6.0) * (k1.pr + 2 * k2.pr + 2 * k3.pr + k4.pr);

  return result;
}

void circular_orbit_constants(double r0, SystemParams sys,
                              double *E_out, double *L_out)
{
  double e_top = 1 - (2 * sys.M / r0);
  double e_bottom = sqrt(1 - (3 * sys.M / r0));
  *E_out = e_top / e_bottom;

  double l_top = sqrt(sys.M * r0);
  double l_bottom = sqrt(1 - (3 * sys.M / r0));
  *L_out = l_top / l_bottom;
}
