#pragma once
#include "SDL3/SDL.h"
#include "simulation.h"

typedef struct {
  Warfield field;
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
