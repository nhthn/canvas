#include <iostream>
#include <SDL2/SDL.h>

constexpr int k_screenWidth = 640;
constexpr int k_screenHeight = 480;

int main(int argc, char** argv) {
    int rendererFlags, windowFlags;
    rendererFlags = SDL_RENDERER_ACCELERATED;
    windowFlags = 0;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
	std::cerr << "Couldn't initialize SDL: " << SDL_GetError() << std::endl;
	exit(1);
    }

    SDL_Window* window = SDL_CreateWindow(
	"Canvas",
	SDL_WINDOWPOS_UNDEFINED,
	SDL_WINDOWPOS_UNDEFINED,
	k_screenWidth,
	k_screenHeight,
	windowFlags
    );
    if (window == nullptr) {
	std::cerr << "Failed to open window: " << SDL_GetError() << std::endl;
	exit(1);
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, rendererFlags);
    if (renderer == nullptr) {
	std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
	exit(1);
    }

    while (true) {
	SDL_SetRenderDrawColor(renderer, 96, 128, 255, 255);
	SDL_RenderClear(renderer);
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
	    if (event.type == SDL_QUIT) {
		exit(0);
	    }
	}
	SDL_RenderPresent(renderer);
    }

    return 0;
}
