#include "simulation.h"
#include <float.h>
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

Soldier *soldier_new(float x, float y, float facing, int team) {
  Soldier *sold = malloc(sizeof(Soldier));
  *sold = (Soldier){.health = 100,
                    .max_health = 100,
                    .is_dead = false,
                    .damage = 10,
                    .attack_cooldown = 1.5,
                    .attack_clock = 0.0,
                    .x = x,
                    .y = y,
                    .speed = 80,
                    .facing_angle = facing,
                    .vision_angle = M_PI_4,
                    .vision_range = 600.0,
                    .range = 200.0,
                    .target = NULL,
                    .team = team};
  return sold;
}

void soldier_free(Soldier *soldier) { free(soldier); }

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

void soldier_chase_target(Soldier *soldier, double delta) {
  if (soldier->target == NULL)
    return;

  if (in_soldier_attack_cone(soldier, soldier->target->x, soldier->target->y)) {
    if (soldier->attack_clock >= soldier->attack_cooldown) {
      soldier->attack_clock = 0;
      soldier->target->health -= soldier->damage;
    } else {
      soldier->attack_clock += delta;
    }
  } else {
    float dx = soldier->target->x - soldier->x,
          dy = soldier->target->y - soldier->y;
    float dist = sqrtf(dx * dx + dy * dy);
    soldier->x += (dx / dist) * soldier->speed * delta;
    soldier->y += (dy / dist) * soldier->speed * delta;
    soldier->facing_angle = angle_norm(atan2f(dy, dx));
  }
}

Warfield warfield_new(void) {
  return (Warfield){.soldiers = calloc(50, sizeof(Soldier *)),
                    .soldiers_count = 0,
                    .soldiers_size = 50,
                    .spawn_queue_count = 0,
                    .spawn_queue_size = 20,
                    .spawn_queue = calloc(20, sizeof(Soldier *))};
}

void warfield_resize(Warfield *field, size_t N) {
  if (N <= field->soldiers_count) {
    printf("Bad warfield resize (count: %zu, tried %zu)", field->soldiers_count,
           N);
    exit(1);
  }

  field->soldiers = realloc(field->soldiers, N * sizeof(Soldier *));
  field->soldiers_size = N;
}

void warfield_append_soldier(Warfield *field, Soldier *soldier) {
  if (field->spawn_queue_count == field->spawn_queue_size) {
    field->spawn_queue_size += 20;
    field->spawn_queue = realloc(field->spawn_queue,
                                 field->spawn_queue_size * sizeof(Soldier *));
  }
  field->spawn_queue_count += 1;
  field->spawn_queue[field->spawn_queue_count - 1] = soldier;
}

void warfield_run_tick(Warfield *field, double delta) {
  for (size_t i = 0; i < field->soldiers_count; ++i) {
    if (field->soldiers[i]->is_dead) {
      continue;
    }

    if (field->soldiers[i]->target != NULL &&
        field->soldiers[i]->target->is_dead) {
      field->soldiers[i]->target = NULL;
    }

    if (field->soldiers[i]->target == NULL) {
      float min_dist = FLT_MAX;
      for (size_t j = 0; j < field->soldiers_count; ++j) {
        if (i == j || field->soldiers[j]->is_dead)
          continue;
        if (field->soldiers[i]->team == field->soldiers[j]->team)
          continue;
        float dx = field->soldiers[j]->x - field->soldiers[i]->x,
              dy = field->soldiers[j]->y - field->soldiers[i]->y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < min_dist) {
          min_dist = dist;
          field->soldiers[i]->target = field->soldiers[j];
        }
      }
    } else {
      soldier_chase_target(field->soldiers[i], delta);
    }
  }

  warfield_spawn(field);

  for (size_t i = 0; i < field->soldiers_count; ++i) {
    if (field->soldiers[i]->health <= 0) {
      field->soldiers[i]->is_dead = true;
    }
  }
}

void warfield_spawn(Warfield *field) {
  size_t count = field->soldiers_count + field->spawn_queue_count;
  if (count >= field->soldiers_size) {
    field->soldiers_size = count + 20;
    field->soldiers =
        realloc(field->soldiers, field->soldiers_size * sizeof(Soldier *));
  }

  Soldier **queue_cursor = field->spawn_queue;
  Soldier **queue_end = field->spawn_queue + field->spawn_queue_count;
  for (size_t i = 0; i < field->soldiers_count; ++i) {
    if (field->soldiers[i]->is_dead) {
      soldier_free(field->soldiers[i]);
      field->soldiers[i] = *queue_cursor;
      queue_cursor += 1;
    }
    if (queue_cursor >= queue_end)
      break;
  }

  int i = 0;
  while (queue_cursor < queue_end) {
    field->soldiers[field->soldiers_count + i] = *queue_cursor;
    queue_cursor += 1;
    i += 1;
  }

  field->soldiers_count += i;

  field->spawn_queue_count = 0;
  field->spawn_queue_size = 20;
  free(field->spawn_queue);
  field->spawn_queue = calloc(field->spawn_queue_size, sizeof(Soldier *));
}

void warfield_free(Warfield *field) {
  for (size_t i = 0; i < field->soldiers_count; ++i) {
    soldier_free(field->soldiers[i]);
  }
  free(field->soldiers);
  field->soldiers = NULL;
  field->soldiers_count = 0;
  field->soldiers_size = 0;

  for (size_t i = 0; i < field->spawn_queue_count; ++i) {
    soldier_free(field->spawn_queue[i]);
  }
  free(field->spawn_queue);
  field->spawn_queue = NULL;
  field->spawn_queue_count = 0;
  field->spawn_queue_size = 0;
}
