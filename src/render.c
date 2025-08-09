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
  Camera camera = camera_new(0, 0);
  float mx, my;
  SDL_GetMouseState(&mx, &my);
  return (GameState){
      .field = field, .camera = camera, .mouse_last_x = mx, .mouse_last_y = my};
}

void gamestate_free(GameState *state) { warfield_free(&state->field); }

Camera camera_new(float x, float y) {
  return (Camera){.x = x, .y = y, .speed = 200, .zoom = 1.0};
}

void camera_displace(Camera *camera, float x, float y) {
  camera->x += x;
  camera->y += y;
}

float worldx(const Camera *camera, float x) {
  return worldlen(camera, x - camera->x);
}

float worldy(const Camera *camera, float y) {
  return worldlen(camera, y - camera->y);
}

float worldlen(const Camera *camera, float w) { return w * camera->zoom; }

float gamex(const Camera *camera, float x) {
  return gamelen(camera, x) + camera->x;
}

float gamey(const Camera *camera, float y) {
  return gamelen(camera, y) + camera->y;
}

float gamelen(const Camera *camera, float w) { return w / camera->zoom; }

void render_soldier(const RenderTarget *target, const Soldier *sold,
                    const GameState *state) {
  if (sold->is_dead)
    return;
  int soldier_size = 30;
  float rectsize = worldlen(&state->camera, soldier_size);
  // Top-left
  float tlx = worldx(&state->camera, sold->x - (float)rectsize / 2),
        tly = worldy(&state->camera, sold->y - (float)rectsize / 2);
  SDL_FRect rect = {.x = tlx, .y = tly, .w = rectsize, .h = rectsize};
  SDL_SetRenderDrawColor(target->rdr, 0xFF, 0x00, 0x00, SDL_ALPHA_OPAQUE);
  SDL_RenderFillRect(target->rdr, &rect);

  // Attacked 0, charged 1
  float attack_progress = sold->attack_clock / sold->attack_cooldown;
  if (attack_progress > 1)
    attack_progress = 1;
  int color_start = 0xAF, color_end = 0xFF;
  int color = (color_end - color_start) * attack_progress + color_start;
  SDL_SetRenderDrawColor(target->rdr, 0x00, 0x00, color, 0x20);
  RSDL_DrawCone(target->rdr, worldx(&state->camera, sold->x),
                worldy(&state->camera, sold->y),
                sold->facing_angle - sold->vision_angle,
                sold->facing_angle + sold->vision_angle,
                worldlen(&state->camera, sold->range));

  // SDL_SetRenderDrawColor(target->rdr, 0x00, 0x00, 0x1F, 0x20);
  // RSDL_DrawCone(target->rdr, worldx(&state->camera, sold->x),
  //               worldy(&state->camera, sold->y), -M_PI, M_PI - 0.001,
  //               worldlen(&state->camera, sold->vision_range));

  SDL_FRect hp_back = {.x = tlx,
                       tly - worldlen(&state->camera, 15),
                       .w = worldlen(&state->camera, soldier_size),
                       .h = worldlen(&state->camera, 10)};
  SDL_SetRenderDrawColor(target->rdr, 0x00, 0x7F, 0x00, SDL_ALPHA_OPAQUE);
  SDL_RenderFillRect(target->rdr, &hp_back);

  SDL_FRect hp_front = hp_back;
  hp_front.w *= (float)sold->health / sold->max_health;
  SDL_SetRenderDrawColor(target->rdr, 0x00, 0xFF, 0x00, SDL_ALPHA_OPAQUE);
  SDL_RenderFillRect(target->rdr, &hp_front);
}

void renderer_events(const RenderTarget *target, const SDL_Event *event,
                     GameState *state) {
  switch (event->type) {
  case SDL_EVENT_MOUSE_WHEEL: {
    float scrolly = event->wheel.y;
    float step = 0.05;
    float min = 0.5, max = 2;
    state->camera.zoom += step * scrolly;
    if (state->camera.zoom < min)
      state->camera.zoom = min;
    if (state->camera.zoom > max)
      state->camera.zoom = max;
    break;
  }
  case SDL_EVENT_MOUSE_BUTTON_DOWN: {
    if (event->button.button == SDL_BUTTON_LEFT) {
      Soldier *sold = soldier_new(gamex(&state->camera, event->button.x),
                                  gamey(&state->camera, event->button.y), 0);
      warfield_append_soldier(&state->field, sold);
    }
    break;
  }
  default:
    break;
  }
}

void renderer_update(const RenderTarget *target, GameState *state,
                     double delta) {
  warfield_run_tick(&state->field, delta);
  float mx, my;
  bool is_reversed = true;
  SDL_MouseButtonFlags mstate = SDL_GetMouseState(&mx, &my);
  if (mstate & SDL_BUTTON_MMASK) {
    float sign = is_reversed ? -1 : 1;
    camera_displace(&state->camera, sign * (mx - state->mouse_last_x),
                    sign * (my - state->mouse_last_y));
  }
  state->mouse_last_x = mx;
  state->mouse_last_y = my;
}

void renderer_draw(const RenderTarget *target, const GameState *state) {
  SDL_SetRenderDrawColor(target->rdr, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(target->rdr);

  for (size_t i = 0; i < state->field.soldiers_count; ++i) {
    render_soldier(target, state->field.soldiers[i], state);
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
