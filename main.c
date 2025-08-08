#include "SDL3/SDL.h"
#include "render.h"

int main(void) {
  SDL_Init(0);

  SDL_Window *win;
  SDL_Renderer *rdr;
  SDL_CreateWindowAndRenderer("Battler", 800, 600, 0, &win, &rdr);

  RenderTarget target = {.win = win, .rdr = rdr};
  renderer_loop(&target);

  SDL_DestroyRenderer(rdr);
  SDL_DestroyWindow(win);
}
