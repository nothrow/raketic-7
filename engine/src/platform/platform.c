#include "platform.h"

#include <Windows.h>
#include <gl/GL.h>
#include <SDL2/SDL.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "cfgmgr32.lib")
#pragma comment(lib, "setupapi.lib")

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600


#define MEMORY_MAX_SIZE (30 * 1024 * 1024)


static SDL_Window* window_;
static SDL_GLContext* context_;
static struct input_state input_state_;

static uint32_t prev_time_;
static double ticks_;

static void* fixed_heap_ = NULL;
static size_t allocated_size_ = 0;

static void _memory_initialize(void) {
  fixed_heap_ = VirtualAlloc(NULL, MEMORY_MAX_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  _ASSERT(fixed_heap_ != NULL);
}

static void _gl_init(void) {
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0, -1.0, 1.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

static void _platform_create_window(void) {
  _VERIFY(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) >= 0, "SDL initialization failed");
  window_ = SDL_CreateWindow("Raketic", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT,
                             SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
  _ASSERT(window_ && "Window creation failed");

  context_ = SDL_GL_CreateContext(window_);
  _ASSERT(context_ && "OpenGL context creation failed");
  SDL_GL_SetSwapInterval(1); // Enable vsync

  platform_clear_memory(&input_state_, sizeof(input_state_));

  _gl_init();
  prev_time_ = SDL_GetTicks();
  ticks_ = 0;

  _memory_initialize();
}

void platform_initialize(void) {
  _memory_initialize();
  _platform_create_window();
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
  }

  int mx = input_state_.mx;
  int my = input_state_.my;

  input_state_.buttons = SDL_GetMouseState(&input_state_.mx, &input_state_.my);

  input_state_.mdx = input_state_.mx - mx;
  input_state_.mdy = input_state_.my - my;

  return true;
}

void* platform_retrieve_memory(size_t memory_size) {
  _ASSERT(allocated_size_ + memory_size < MEMORY_MAX_SIZE);

  void* ptr = (char*)fixed_heap_ + allocated_size_;
  allocated_size_ += memory_size;
  return ptr;
}
