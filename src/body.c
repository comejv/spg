#include "body.h"

void init_body(Body *b, Vector3 position, double mass, double radius, Color color)
{
  b->position = position;
  b->velocity = (Vector3) {0.0F, 0.0F, 0.0F};
  b->mass = mass;
  b->radius = radius;
  b->color = color;
}
