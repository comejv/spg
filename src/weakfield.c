// weakfield.c
#include "weakfield.h"
#include "simulation.h"

static inline double clampd(double x, double a, double b)
{
  if (x < a)
    return a;
  if (x > b)
    return b;
  return x;
}

void potential_and_gradU(const Simulation *sim, Vec3d x, double *U_out, Vec3d *gradU_out)
{
  double U = 0.0;
  Vec3d gradU = V3d(0, 0, 0);

  for (int i = 0; i < sim->body_count; i++)
  {
    const Body *b = &sim->bodies[i];
    Vec3d xb = v3d_from_v3(b->position);

    Vec3d d = v3d_sub(x, xb);
    double r2 = v3d_len2(d);

    // Softening to avoid singularities. Tie it to radius if present.
    double eps = (double) b->radius * 0.5;
    if (eps < 1e-6)
      eps = 1e-6;

    r2 += eps * eps;

    double r = sqrt(r2);
    double invr = 1.0 / r;
    double invr3 = invr / r2;

    U += sim->G * b->mass * invr;

    // ∇(1/r) = -d / r^3
    gradU = v3d_add(gradU, v3d_scale(d, -sim->G * b->mass * invr3));
  }

  *U_out = U;
  *gradU_out = gradU;
}

static WeakFieldState weakfield_deriv(WeakFieldState s, const Simulation *sim)
{
  WeakFieldState ds = {0};

  double U = 0.0;
  Vec3d gradU = V3d(0, 0, 0);
  potential_and_gradU(sim, s.x, &U, &gradU);

  double c = sim->c;
  double c2 = c * c;
  double invc2 = 1.0 / c2;

  // A = 1 - 2U/c^2, B = 1 + 2U/c^2
  double A = 1.0 - (2.0 * U * invc2);
  double B = 1.0 + (2.0 * U * invc2);

  // Avoid pathological A,B (weak-field assumption violated).
  A = clampd(A, 1e-6, 1e6);
  B = clampd(B, 1e-6, 1e6);

  // g^tt = -1/(A c^2)
  double gtt = -1.0 / (A * c2);

  // dt/dλ = g^tt p_t
  ds.t = gtt * s.pt;

  // dx^i/dλ = g^ij p_j = (1/B) p_i
  ds.x = v3d_scale(s.p, 1.0 / B);

  // dp_i/dλ = -1/2 ∂_i g^{αβ} p_α p_β
  // For this metric (static, diagonal), this simplifies to:
  // dp = (∇U) * [ (p_t^2)/(c^4 A^2) + (p^2)/(c^2 B^2) ]
  double p2 = v3d_len2(s.p);

  double c4 = c2 * c2;
  double coeff = ((s.pt * s.pt) / (c4 * A * A)) + (p2 / (c2 * B * B));

  ds.p = v3d_scale(gradU, coeff);

  // We treat pt constant in quasi-static approximation.
  ds.pt = 0.0;

  return ds;
}

void weakfield_project_to_constraint(WeakFieldState *s, const Simulation *sim, bool is_massive)
{
  double U = 0.0;
  Vec3d gradU = V3d(0, 0, 0);
  potential_and_gradU(sim, s->x, &U, &gradU);

  double c = sim->c;
  double c2 = c * c;

  double A = 1.0 - (2.0 * U / c2);
  double B = 1.0 + (2.0 * U / c2);

  if (A < 1e-9)
    A = 1e-9;
  if (B < 1e-9)
    B = 1e-9;

  double gtt = -1.0 / (A * c2);
  double target = is_massive ? (-c2) : 0.0;

  // Need p^2 such that: gtt pt^2 + (1/B) p^2 = target
  double p2_des = B * (target - gtt * s->pt * s->pt);
  if (p2_des < 0.0)
    return;   // can't project (weak-field broken or pt too small)

  double p2 = v3d_len2(s->p);
  if (p2 < 1e-30)
    return;

  double scale = sqrt(p2_des / p2);
  s->p = v3d_scale(s->p, scale);
}

WeakFieldState weakfield_rk4_step(WeakFieldState s, const Simulation *sim, double h)
{
  WeakFieldState k1 = weakfield_deriv(s, sim);

  WeakFieldState s2 = s;
  s2.t += 0.5 * h * k1.t;
  s2.x = v3d_add(s2.x, v3d_scale(k1.x, 0.5 * h));
  s2.p = v3d_add(s2.p, v3d_scale(k1.p, 0.5 * h));
  WeakFieldState k2 = weakfield_deriv(s2, sim);

  WeakFieldState s3 = s;
  s3.t += 0.5 * h * k2.t;
  s3.x = v3d_add(s3.x, v3d_scale(k2.x, 0.5 * h));
  s3.p = v3d_add(s3.p, v3d_scale(k2.p, 0.5 * h));
  WeakFieldState k3 = weakfield_deriv(s3, sim);

  WeakFieldState s4 = s;
  s4.t += h * k3.t;
  s4.x = v3d_add(s4.x, v3d_scale(k3.x, h));
  s4.p = v3d_add(s4.p, v3d_scale(k3.p, h));
  WeakFieldState k4 = weakfield_deriv(s4, sim);

  WeakFieldState out = s;
  out.t += (h / 6.0) * (k1.t + 2.0 * k2.t + 2.0 * k3.t + k4.t);

  Vec3d dx = v3d_add(
      v3d_add(v3d_scale(k1.x, 1.0), v3d_scale(k2.x, 2.0)),
      v3d_add(v3d_scale(k3.x, 2.0), v3d_scale(k4.x, 1.0)));
  out.x = v3d_add(out.x, v3d_scale(dx, h / 6.0));

  Vec3d dp = v3d_add(
      v3d_add(v3d_scale(k1.p, 1.0), v3d_scale(k2.p, 2.0)),
      v3d_add(v3d_scale(k3.p, 2.0), v3d_scale(k4.p, 1.0)));
  out.p = v3d_add(out.p, v3d_scale(dp, h / 6.0));

  // pt unchanged
  return out;
}

double weakfield_constraint_err(const WeakFieldState *s, const Simulation *sim, bool is_massive)
{
  double U = 0.0;
  Vec3d gradU = V3d(0, 0, 0);
  potential_and_gradU(sim, s->x, &U, &gradU);

  double c = sim->c;
  double c2 = c * c;

  double A = 1.0 - (2.0 * U / c2);
  double B = 1.0 + (2.0 * U / c2);

  if (A < 1e-12)
    A = 1e-12;
  if (B < 1e-12)
    B = 1e-12;

  double gtt = -1.0 / (A * c2);
  double invB = 1.0 / B;

  double twoH = (gtt * (s->pt * s->pt)) + (invB * v3d_len2(s->p));

  // Constraint: pμ p^μ = twoH = -m^2 c^2 (massive), 0 (photon)
  double target = is_massive ? (-c2) : 0.0;
  return twoH - target;
}
