// vec3d.h
#pragma once

#include <math.h>
#include <raylib.h>

typedef struct Vec3d
{
  double x;
  double y;
  double z;
} Vec3d;

static inline Vec3d V3d(double x, double y, double z)
{
  Vec3d v = {x, y, z};
  return v;
}

static inline Vec3d v3d_add(Vec3d a, Vec3d b)
{
  return V3d(a.x + b.x, a.y + b.y, a.z + b.z);
}

static inline Vec3d v3d_sub(Vec3d a, Vec3d b)
{
  return V3d(a.x - b.x, a.y - b.y, a.z - b.z);
}

static inline Vec3d v3d_scale(Vec3d a, double s)
{
  return V3d(a.x * s, a.y * s, a.z * s);
}

static inline double v3d_dot(Vec3d a, Vec3d b)
{
  return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

static inline double v3d_len2(Vec3d a)
{
  return v3d_dot(a, a);
}

static inline double v3d_len(Vec3d a)
{
  return sqrt(v3d_len2(a));
}

static inline Vec3d v3d_norm(Vec3d a)
{
  double l = v3d_len(a);
  if (l <= 0.0)
    return V3d(0, 0, 0);
  return v3d_scale(a, 1.0 / l);
}

static inline Vector3 v3d_to_v3(Vec3d a)
{
  Vector3 v = {(float) a.x, (float) a.y, (float) a.z};
  return v;
}

static inline Vec3d v3d_from_v3(Vector3 a)
{
  return V3d((double) a.x, (double) a.y, (double) a.z);
}
