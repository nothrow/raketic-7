#include <SDL2/SDL.h>

#include "plat.h"
#include "clib/clib.h"



#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "cfgmgr32.lib")
#pragma comment(lib, "setupapi.lib")


static SDL_Window* window_;
static SDL_GLContext* context_;

void plat_init(void) {
	_VERIFY(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) >= 0, "SDL initialization failed");
	window_ = SDL_CreateWindow("Raketic", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
	_ASSERT(window_ && "Window creation failed");


	context_ = SDL_GL_CreateContext(window_);
	_ASSERT(context_ && "OpenGL context creation failed");
}

int plat_loop(void)
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			return 0;
		}
	}

	SDL_GL_SwapWindow(window_);
	return 1;
}
