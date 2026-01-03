#include "platform.h"

#include <Windows.h>
#include <gl/GL.h>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "opengl32.lib")
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

bool platform_loop(void) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      return false;
    }
    else if (event.type == SDL_MOUSEMOTION) {
      messaging_send(ENTITY_TYPE_CONTROLLER, RECIPIENT_ID_BROADCAST, CREATE_MESSAGE(MESSAGE_CONTROLLER_MOUSEMOVE, event.motion.xrel, event.motion.yrel));
    }
  }

  int mx = input_state_.mx;
  int my = input_state_.my;

  input_state_.buttons = SDL_GetMouseState(&input_state_.mx, &input_state_.my);

  input_state_.mdx = input_state_.mx - mx;
  input_state_.mdy = input_state_.my - my;

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
