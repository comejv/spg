#include "particle.h"
#include "simulation.h"
#include "vec3d.h"

void init_particle(Particle *p, SystemParams sys, double r0, double phi0, double pr0, bool massive)
{
  p->active = true;
  p->trail_head = 0;
  p->trail_full = false;

  // Clear trail
  for (int i = 0; i < MAX_TRAIL_LENGTH; ++i)
  {
    p->trail[i] = (Vector3) {0};
  }

  p->state.t = 0.0;
  p->state.r = r0;
  p->state.phi = phi0;
  p->state.pr = pr0;

  p->params.is_massive = massive;

  if (massive)
  {
    p->color = BLUE;
    // Calculate circular orbit parameters for stable start
    // We use the circular orbit L, but we must recalculate E if pr0 != 0
    double E_circ = 0.0;
    circular_orbit_constants(r0, sys, &E_circ, &p->params.L);

    // Recalculate E to satisfy constraint: H = -c^2/2
    // E = c * sqrt( f * (c^2 + f*pr^2 + L^2/r^2) )
    double rs = 2.0 * sys.G * sys.M / (sys.c * sys.c);
    double f = 1.0 - (rs / r0);
    double c2 = sys.c * sys.c;
    double L2 = p->params.L * p->params.L;

    double term_inside = c2 + (f * pr0 * pr0) + (L2 / (r0 * r0));
    p->params.E = sys.c * sqrt(f * term_inside);
  }
  else
  {
    p->color = RED;
    // Photons
    // We start with a baseline L for impact parameter b ~ 5.5 M
    double E_base = 1.0;
    double b = 5.5 * sys.M;
    p->params.L = b * E_base;   // This L is just an initial guess for magnitude

    // But we must satisfy constraint H = 0 with given pr0.
    // E = c * sqrt( f * ( f*pr^2 + L^2/r^2 ) )
    double rs = 2.0 * sys.G * sys.M / (sys.c * sys.c);
    double f = 1.0 - (rs / r0);
    double L2 = p->params.L * p->params.L;

    double term_inside = (f * pr0 * pr0) + (L2 / (r0 * r0));
    p->params.E = sys.c * sqrt(f * term_inside);
  }
}

static inline double photon_pr_from_b(double r0, SystemParams sys, double E, double b, bool inward)
{
  double rs = rs_schw(sys);
  double f = 1.0 - (rs / r0);
  double c2 = sys.c * sys.c;

  // L = bE
  double L = b * E;

  double inside = (E * E) / (f * c2) - (L * L) / (r0 * r0);
  double pr2 = inside / f;

  if (pr2 < 0.0)
    pr2 = 0.0;   // no real radial momentum for that (r0,b,E)

  double pr = sqrt(pr2);
  return inward ? -pr : pr;
}

void init_particle_photon(Particle *p,
                          SystemParams sys,
                          double r0,
                          double phi0,
                          double b,        // impact parameter
                          bool inward,     // radial direction
                          double E_scale   // choose 1.0 unless you care about dt/dλ scale
)
{
  p->active = true;
  p->trail_head = 0;
  p->trail_full = false;
  for (int i = 0; i < MAX_TRAIL_LENGTH; i++)
    p->trail[i] = (Vector3) {0};

  p->color = RED;
  p->params.is_massive = false;

  p->state.t = 0.0;
  p->state.r = r0;
  p->state.phi = phi0;

  p->params.E = E_scale;
  p->params.L = b * E_scale;
  p->state.pr = photon_pr_from_b(r0, sys, p->params.E, b, inward);
}

static inline void particle_clear_trail(Particle *p)
{
  p->trail_head = 0;
  p->trail_full = false;
  for (int i = 0; i < MAX_TRAIL_LENGTH; i++)
    p->trail[i] = (Vector3) {0};
}

void particle_schw_to_weakfield(Particle *p, const Simulation *sim)
{
  double r = p->state.r;
  double phi = p->state.phi;

  double cphi = cos(phi);
  double sphi = sin(phi);

  Vec3d rhat = V3d(cphi, 0.0, sphi);
  Vec3d phihat = V3d(-sphi, 0.0, cphi);

  double pr = p->state.pr;
  double L = p->params.L;

  // Flat-space mapping: p = pr * rhat + (L/r) * phihat
  Vec3d x = V3d(r * cphi, 0.0, r * sphi);
  Vec3d mom = v3d_add(v3d_scale(rhat, pr), v3d_scale(phihat, L / r));

  p->wf.t = p->state.t;
  p->wf.x = x;
  p->wf.p = mom;

  // Recalculate pt to satisfy constraint in Weak Field metric
  double U = 0.0;
  Vec3d gradU = {0};
  potential_and_gradU(sim, x, &U, &gradU);

  double c = sim->c;
  double c2 = c * c;
  double A = 1.0 - (2.0 * U / c2);
  double B = 1.0 + (2.0 * U / c2);

  // Safety clamp
  if (A < 1e-9)
    A = 1e-9;
  if (B < 1e-9)
    B = 1e-9;

  double p2 = v3d_len2(mom);
  double target_c2 = p->params.is_massive ? c2 : 0.0;

  // pt^2 = A * c^2 * (target_c2 + p^2/B)
  // pt = -sqrt(...)

  double term = target_c2 + (p2 / B);
  if (term < 0)
    term = 0;   // Should not happen for physical states

  p->wf.pt = -c * sqrt(A * term);

  p->model = PARTICLE_MODEL_WEAKFIELD;
}

void particle_weakfield_to_schw(Particle *p, SystemParams sys)
{
  // Assumes near-equatorial motion (y ~ 0)
  double x = p->wf.x.x;
  double z = p->wf.x.z;
  double r = sqrt((x * x) + (z * z));
  if (r < 1e-9)
    r = 1e-9;

  double phi = atan2(z, x);
  double cphi = cos(phi);
  double sphi = sin(phi);

  Vec3d rhat = V3d(cphi, 0.0, sphi);
  Vec3d phihat = V3d(-sphi, 0.0, cphi);

  double pr = v3d_dot(p->wf.p, rhat);
  double pphi = v3d_dot(p->wf.p, phihat);
  double L = r * pphi;

  p->state.t = p->wf.t;
  p->state.r = r;
  p->state.phi = phi;
  p->state.pr = pr;

  p->params.L = L;

  // Recalculate E using Schwarzschild constraint
  // E = c * sqrt( f * (target_c2 + f*pr^2 + L^2/r^2) )
  double rs = 2.0 * sys.G * sys.M / (sys.c * sys.c);
  double f = 1.0 - (rs / r);
  if (f < 1e-9)
    f = 1e-9;

  double c2 = sys.c * sys.c;
  double target_c2 = p->params.is_massive ? c2 : 0.0;

  double term = target_c2 + (f * pr * pr) + (L * L / (r * r));
  if (term < 0)
    term = 0;

  p->params.E = sys.c * sqrt(f * term);

  p->model = PARTICLE_MODEL_SCHW;
}

static inline double recompute_E_schw(SystemParams sys, double r, double pr, double L, bool massive)
{
  double rs = rs_schw(sys);
  double f = 1.0 - (rs / r);
  if (f < 1e-12)
    f = 1e-12;

  double c2 = sys.c * sys.c;

  if (massive)
  {
    // E = c * sqrt( f * (c^2 + f pr^2 + L^2/r^2) )
    double inside = c2 + (f * pr * pr) + ((L * L) / (r * r));
    return sys.c * sqrt(f * inside);
  }
  else
  {
    // E = c * sqrt( f * (f pr^2 + L^2/r^2) )
    double inside = (f * pr * pr) + ((L * L) / (r * r));
    return sys.c * sqrt(f * inside);
  }
}

void enforce_constraint_schwarzschild(Particle *p, SystemParams sys)
{
  p->params.E = recompute_E_schw(
      sys, p->state.r, p->state.pr, p->params.L, p->params.is_massive);
}
