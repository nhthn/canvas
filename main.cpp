#include <iostream>
#include <SDL2/SDL.h>

constexpr int k_screenWidth = 640;
constexpr int k_screenHeight = 480;

class App {
public:
    App()
    {
	initSDL();
	initWindow();
	initRenderer();
	mainLoop();
    }

private:
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;

    void initSDL()
    {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
	    std::cerr << "Couldn't initialize SDL: " << SDL_GetError() << std::endl;
	    exit(1);
	}
    }

    void initWindow()
    {
	int windowFlags = 0;
	m_window = SDL_CreateWindow(
	    "Canvas",
	    SDL_WINDOWPOS_UNDEFINED,
	    SDL_WINDOWPOS_UNDEFINED,
	    k_screenWidth,
	    k_screenHeight,
	    windowFlags
	);
	if (m_window == nullptr) {
	    std::cerr << "Failed to open window: " << SDL_GetError() << std::endl;
	    exit(1);
	}
    }

    void initRenderer()
    {
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	int rendererFlags = SDL_RENDERER_ACCELERATED;
	m_renderer = SDL_CreateRenderer(m_window, -1, rendererFlags);
	if (m_renderer == nullptr) {
	    std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
	    exit(1);
	}
    }

    void mainLoop()
    {
	while (true) {
	    SDL_SetRenderDrawColor(m_renderer, 96, 128, 255, 255);
	    SDL_RenderClear(m_renderer);
	    SDL_Event event;
	    while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
		    exit(0);
		}
	    }
	    SDL_RenderPresent(m_renderer);
	}
    }
};

int main(int argc, char** argv) {
    App app;
    return 0;
}
