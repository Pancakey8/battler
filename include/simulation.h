#pragma once
#include <stdbool.h>
#include <stddef.h>

float angle_norm(float angle);

typedef struct _soldier {
  int health, max_health;
  bool is_dead;
  int damage;
  float attack_cooldown;
  float attack_clock;
  float x, y;
  float speed;
  float facing_angle;
  float vision_angle;
  float vision_range;
  float range;
  struct _soldier *target;
  int team;
} Soldier;

Soldier *soldier_new(float x, float y, float facing, int team);
bool in_soldier_attack_cone(const Soldier *soldier, float x, float y);
void soldier_free(Soldier *soldier);

typedef struct {
  Soldier **soldiers;
  size_t soldiers_count;
  size_t soldiers_size;
} Warfield;

Warfield warfield_new(void);
void warfield_append_soldier(Warfield *field, Soldier *soldier);
void warfield_run_tick(Warfield *field, double delta);
void warfield_free(Warfield *field);
