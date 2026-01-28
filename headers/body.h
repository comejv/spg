#pragma once

#include <raylib.h>

typedef struct
{
  Vector3 position;
  Vector3 velocity;
  double mass;
  double radius;   // Schwarzschild radius or physical radius
  Color color;
} Body;

void InitBody(Body *b, Vector3 position, double mass, double radius, Color color);
