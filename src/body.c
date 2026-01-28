#include "body.h"

void InitBody(Body *b, Vector3 position, double mass, double radius, Color color)
{
  b->position = position;
  b->velocity = (Vector3) {0.0F, 0.0F, 0.0F};
  b->mass = mass;
  b->radius = radius;
  b->color = color;
}
