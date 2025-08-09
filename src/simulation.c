#include "simulation.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

float angle_norm(float angle) {
  angle = fmodf(angle, 2 * M_PI);
  if (angle < 0.0) {
    angle += 2.0 * M_PI;
  }

  return angle;
}

Soldier soldier_new(float x, float y, float facing) {
  return (Soldier){.health = 100,
                   .max_health = 100,
                   .damage = 5,
                   .attack_cooldown = 2.0,
                   .attack_clock = 0.0,
                   .x = x,
                   .y = y,
                   .speed = 80,
                   .facing_angle = facing,
                   .vision_angle = M_PI_4,
                   .vision_range = 600.0,
                   .range = 200.0};
}

// Consider facing +- vision
bool in_soldier_attack_cone(const Soldier *soldier, float x, float y) {
  float dy = y - soldier->y, dx = x - soldier->x;
  float angle = angle_norm(atan2f(dy, dx));
  float dist = sqrtf(dx * dx + dy * dy);

  float vf0 = angle_norm(soldier->facing_angle - soldier->vision_angle),
        vf1 = angle_norm(soldier->facing_angle + soldier->vision_angle);

  if (vf0 > vf1) {
    return dist <= soldier->range &&
           ((vf0 <= angle && angle < 2 * M_PI) || (0 <= angle && angle <= vf1));
  } else {
    return dist <= soldier->range && vf0 <= angle && angle <= vf1;
  }
}

void soldier_approach_target(Soldier *soldier, const Soldier *target,
                             double delta) {
  float dy = target->y - soldier->y, dx = target->x - soldier->x;
  float dist = sqrtf(dx * dx + dy * dy);
  float margin = 5;

  if (dist > target->range - margin) {
    soldier->x += (dx / dist) * soldier->speed * delta;
    soldier->y += (dy / dist) * soldier->speed * delta;
    soldier->facing_angle = atan2f(dy, dx);
  }
}

Warfield warfield_new(void) {
  return (Warfield){.soldiers = calloc(50, sizeof(Soldier)),
                    .soldiers_count = 0,
                    .soldiers_size = 50};
}

void warfield_resize(Warfield *field, size_t N) {
  if (N <= field->soldiers_count) {
    printf("Bad warfield resize (count: %zu, tried %zu)", field->soldiers_count,
           N);
    exit(1);
  }

  field->soldiers = realloc(field->soldiers, N * sizeof(Soldier));
  field->soldiers_size = N;
}

void warfield_append_soldier(Warfield *field, Soldier soldier) {
  if (field->soldiers_count == field->soldiers_size) {
    warfield_resize(field, field->soldiers_size + 50);
  }

  field->soldiers_count += 1;
  field->soldiers[field->soldiers_count - 1] = soldier;
}

void warfield_run_tick(Warfield *field, double delta) {
  for (size_t i = 0; i < field->soldiers_count; ++i) {
    for (size_t j = 0; j < field->soldiers_count; ++j) {
      if (i == j)
        continue;
      if (field->soldiers[i].attack_clock >=
          field->soldiers[i].attack_cooldown) {
        if (in_soldier_attack_cone(&field->soldiers[i], field->soldiers[j].x,
                                   field->soldiers[j].y)) {
          field->soldiers[j].health -= field->soldiers[i].damage;
          field->soldiers[i].attack_clock = 0.0;
        }
      } else {
        field->soldiers[i].attack_clock += delta;
      }

      float disty = field->soldiers[j].y - field->soldiers[i].y,
            distx = field->soldiers[j].x - field->soldiers[i].x;
      float dist = sqrtf(distx * distx + disty * disty);
      if (dist <= field->soldiers[i].vision_range) {
        soldier_approach_target(&field->soldiers[i], &field->soldiers[j],
                                delta);
      }
    }
  }
}

void warfield_free(Warfield *field) {
  free(field->soldiers);
  field->soldiers = NULL;
  field->soldiers_count = 0;
  field->soldiers_size = 0;
}
