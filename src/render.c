#include "render.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

void RSDL_DrawCone(SDL_Renderer *renderer, float cx, float cy,
                   float angle_start, float angle_end, float radius) {
  angle_start = angle_norm(angle_start);
  angle_end = angle_norm(angle_end);

  if (angle_start > angle_end) {
    angle_end += 2 * M_PI;
  }

  float step = 0.1f;
  int arc_points = floorf((angle_end - angle_start) / step) + 1;
  int triangles = arc_points - 1;
  int vertex_count = triangles * 3;

  SDL_Vertex *vertices = malloc(vertex_count * sizeof(SDL_Vertex));

  SDL_FColor col;
  SDL_GetRenderDrawColorFloat(renderer, &col.r, &col.g, &col.b, &col.a);

  int v = 0;
  for (int i = 0; i < triangles; i++) {
    float ang1 = angle_start + i * step;
    float ang2 = angle_start + (i + 1) * step;
    if (i == triangles - 1) {
      ang2 = angle_end;
    }

    vertices[v] = (SDL_Vertex){.position = {.x = cx, .y = cy}, .color = col};
    v += 1;

    vertices[v] = (SDL_Vertex){.position = {.x = cx + radius * cosf(ang1),
                                            .y = cy + radius * sinf(ang1)},
                               .color = col};
    v += 1;

    vertices[v] = (SDL_Vertex){.position = {.x = cx + radius * cosf(ang2),
                                            .y = cy + radius * sinf(ang2)},
                               .color = col};
    v += 1;
  }

  SDL_RenderGeometry(renderer, NULL, vertices, vertex_count, NULL, 0);

  free(vertices);
}

GameState gamestate_new(void) {
  Warfield field = warfield_new();
  warfield_append_soldier(&field, soldier_new(50, 200, 0));
  warfield_append_soldier(&field, soldier_new(200, 200, M_PI));
  return (GameState){.field = field};
}

void gamestate_free(GameState *state) { warfield_free(&state->field); }

void renderer_events(const RenderTarget *target, const SDL_Event *event,
                     GameState *state) {
  switch (event->type) {
    // TODO
  default:
    break;
  }
}

void renderer_update(const RenderTarget *target, GameState *state,
                     double delta) {
  warfield_run_tick(&state->field, delta);
}

void renderer_draw(const RenderTarget *target, const GameState *state) {
  SDL_SetRenderDrawColor(target->rdr, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(target->rdr);

  for (size_t i = 0; i < state->field.soldiers_count; ++i) {
    Soldier *sold = &state->field.soldiers[i];

    int soldier_size = 30;
    // Top-left
    float tlx = sold->x - (float)soldier_size / 2,
          tly = sold->y - (float)soldier_size / 2;
    SDL_FRect rect = {.x = tlx, .y = tly, .w = soldier_size, .h = soldier_size};
    SDL_SetRenderDrawColor(target->rdr, 0xFF, 0x00, 0x00, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(target->rdr, &rect);

    // Just attacked 0 -> 1
    // About to attack >=1 -> 0.5
    float attack_progress = sold->attack_clock / sold->attack_cooldown;
    if (attack_progress > 1)
      attack_progress = 1;
    float range_mult = (1 - attack_progress / 2);
    float d_angle = sold->vision_angle * range_mult;
    SDL_SetRenderDrawColor(target->rdr, 0x00, 0x00, 0xFF, 0x20);
    RSDL_DrawCone(target->rdr, sold->x, sold->y, sold->facing_angle - d_angle,
                  sold->facing_angle + d_angle, sold->range);

    SDL_FRect hp_back = {.x = tlx, tly - 15, .w = soldier_size, .h = 10};
    SDL_SetRenderDrawColor(target->rdr, 0x00, 0x7F, 0x00, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(target->rdr, &hp_back);

    SDL_FRect hp_front = hp_back;
    hp_front.w *= (float)sold->health / sold->max_health;
    SDL_SetRenderDrawColor(target->rdr, 0x00, 0xFF, 0x00, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(target->rdr, &hp_front);
  }

  SDL_RenderPresent(target->rdr);
}

void renderer_loop(const RenderTarget *target) {
  bool is_running = true;
  Uint64 last_tick = SDL_GetPerformanceCounter();
  GameState state = gamestate_new();

  SDL_SetRenderDrawBlendMode(target->rdr, SDL_BLENDMODE_BLEND);

  while (is_running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      // Handle quit separately since it's fundamental.
      if (event.type == SDL_EVENT_QUIT) {
        is_running = false;
      }
      renderer_events(target, &event, &state);
    }

    Uint64 this_tick = SDL_GetPerformanceCounter();
    double delta =
        (double)(this_tick - last_tick) / SDL_GetPerformanceFrequency();
    renderer_update(target, &state, delta);
    last_tick = this_tick;

    renderer_draw(target, &state);
  }

  gamestate_free(&state);
}
