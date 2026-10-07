#include "app/window.h"

#include <SDL.h>

#include <cstdio>

int run_window(const Scene& scene, Renderer& renderer, int width, int height) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Window* window = SDL_CreateWindow("soft-raster", SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED, width, height, 0);
  SDL_Renderer* sdl = window ? SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC) : nullptr;
  // RGBA32 = bytes R, G, B, A in memory, matching pack_rgba().
  SDL_Texture* texture = sdl ? SDL_CreateTexture(sdl, SDL_PIXELFORMAT_RGBA32,
                                                 SDL_TEXTUREACCESS_STREAMING, width, height)
                             : nullptr;
  if (!texture) {
    std::fprintf(stderr, "SDL setup failed: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  Framebuffer fb(width, height);
  RenderOptions& opt = renderer.options;
  double smoothed_ms = 0;
  bool running = true, paused = false;
  int frame = 0;

  for (int tick = 0; running; ++tick) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) running = false;
      if (e.type != SDL_KEYDOWN) continue;
      switch (e.key.keysym.sym) {
        case SDLK_ESCAPE: running = false; break;
        case SDLK_SPACE: paused = !paused; break;
        case SDLK_p: opt.perspective_correct = !opt.perspective_correct; break;
        case SDLK_c: opt.cull_backfaces = !opt.cull_backfaces; break;
        case SDLK_l: opt.lighting = !opt.lighting; break;
        default: break;
      }
    }

    FrameStats stats = renderer.render(fb, scene, frame);
    double ms = stats.vertex_ms + stats.raster_ms;
    smoothed_ms = tick == 0 ? ms : 0.95 * smoothed_ms + 0.05 * ms;
    if (!paused) ++frame;

    SDL_UpdateTexture(texture, nullptr, fb.color.data(), width * 4);
    SDL_RenderClear(sdl);
    SDL_RenderCopy(sdl, texture, nullptr, nullptr);
    SDL_RenderPresent(sdl);

    if (tick % 20 == 0) {
      char title[160];
      std::snprintf(title, sizeof(title), "soft-raster | %.2f ms | persp %s  cull %s  light %s",
                    smoothed_ms, opt.perspective_correct ? "on" : "off",
                    opt.cull_backfaces ? "on" : "off", opt.lighting ? "on" : "off");
      SDL_SetWindowTitle(window, title);
    }
  }

  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(sdl);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
