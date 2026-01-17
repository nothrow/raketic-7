#include "platform.h"

#include <Windows.h>
#include <gl/GL.h>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "cfgmgr32.lib")
#pragma comment(lib, "setupapi.lib")

static SDL_Window* window_;
static SDL_GLContext* context_;
static struct input_state input_state_;

static uint32_t prev_time_;
static double ticks_;

void _memory_initialize(void);
void _gl_initialize(void);
void _math_initialize(void);

#include "entity/types.h"
#include "messaging/messaging.h"

static void _platform_create_window(void) {
  int sdl_init = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  _ASSERT(sdl_init >= 0 && "SDL initialization failed");
  window_ = SDL_CreateWindow("Raketic", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT,
                             SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
  _ASSERT(window_ && "Window creation failed");

  context_ = SDL_GL_CreateContext(window_);
  _ASSERT(context_ && "OpenGL context creation failed");
  SDL_GL_SetSwapInterval(1); // Enable vsync

  SDL_SetRelativeMouseMode(SDL_TRUE);

  platform_clear_memory(&input_state_, sizeof(input_state_));

  prev_time_ = SDL_GetTicks();
  ticks_ = 0;
}

void platform_initialize(void) {
  _memory_initialize();
  _platform_create_window();
  _gl_initialize();
  _math_initialize();
}

const struct input_state* platform_get_input_state(void) {
  return &input_state_;
}

bool platform_input_is_key_down(enum keys key) {
  return input_state_.keyPressed[key];
}

bool platform_input_is_button_down(enum buttons button) {
  if (button == BUTTON_LEFT) {
    return SDL_BUTTON(SDL_BUTTON_LEFT) & input_state_.buttons;
  }
  else if (button == BUTTON_RIGHT) {
    return SDL_BUTTON(SDL_BUTTON_RIGHT) & input_state_.buttons;
  }
  _ASSERT(0 && "invalid button");
  return false;
}

void platform_frame_start(void) {
  glClear(GL_COLOR_BUFFER_BIT);

  uint32_t now = SDL_GetTicks();
  ticks_ += now - prev_time_;
  prev_time_ = now;

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void platform_frame_end(void) {
  SDL_GL_SwapWindow(window_);
  SDL_Delay(0);
}

bool platform_tick_pending(void) {
  if (ticks_ >= TICK_MS) {
    ticks_ -= TICK_MS;
    return true;
  }
  return false;
}

static void _map_keystates(const Uint8* k)
{
  platform_clear_memory(input_state_.keyPressed, sizeof(input_state_.keyPressed));
  input_state_.keyPressed[KEY_SPACE] = k[SDL_SCANCODE_SPACE];
}

bool platform_loop(void) {
  // Reset delta každý frame
  input_state_.mdx = 0;
  input_state_.mdy = 0;

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_QUIT:
      return false;
    case SDL_MOUSEMOTION:
      input_state_.mdx += event.motion.xrel;
      input_state_.mdy += event.motion.yrel;
      input_state_.mx = event.motion.x;
      input_state_.my = event.motion.y;
      break;
    case SDL_KEYDOWN:
      if (event.key.keysym.sym == SDLK_ESCAPE) {
        return false;
      }
      break;
    }
  }

  input_state_.buttons = SDL_GetMouseState(NULL, NULL);
  _map_keystates(SDL_GetKeyboardState(NULL));
  return true;
}

// defined in main.c
int run(void);

int __cdecl main(int argc, char** argv) {
  SDL_SetMainReady();
  (argc);
  (argv);
  return run();
}
