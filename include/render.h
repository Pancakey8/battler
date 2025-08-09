#pragma once
#include "SDL3/SDL.h"
#include "simulation.h"

typedef struct {
  float x, y;
  float speed;
  float zoom;
} Camera;

Camera camera_new(float x, float y);
void camera_displace(Camera *camera, float x, float y);
float worldx(const Camera *camera, float x);
float worldy(const Camera *camera, float y);
float worldlen(const Camera *camera, float w);
float gamex(const Camera *camera, float x);
float gamey(const Camera *camera, float y);
float gamelen(const Camera *camera, float w);

typedef struct {
  Warfield field;
  Camera camera;
  float mouse_last_x, mouse_last_y;
} GameState;

GameState gamestate_new(void);

void gamestate_free(GameState *state);

typedef struct {
  SDL_Window *win;
  SDL_Renderer *rdr;
} RenderTarget;

void renderer_events(const RenderTarget *target, const SDL_Event *event,
                     GameState *state);

void renderer_update(const RenderTarget *target, GameState *state,
                     double delta);

void renderer_draw(const RenderTarget *target, const GameState *state);

void renderer_loop(const RenderTarget *target);
