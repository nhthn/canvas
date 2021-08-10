#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

constexpr int k_windowWidth = 640;
constexpr int k_windowHeight = 480;

class App {
public:
    App()
    {
	initSDL();
	initWindow();
	initRenderer();

	m_texture = SDL_CreateTexture(
	    m_renderer,
	    SDL_PIXELFORMAT_ARGB8888,
	    SDL_TEXTUREACCESS_STATIC,
	    k_windowWidth,
	    k_windowHeight
	);

	m_pixels = new Uint32[k_windowHeight * k_windowWidth];
	memset(m_pixels, 255, k_windowHeight * k_windowWidth * sizeof(Uint32));

	mainLoop();
    }

    ~App()
    {
	delete m_pixels;
    }

private:
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    SDL_Texture* m_texture;
    Uint32* m_pixels;
    bool m_leftMouseButtonDown;

    void initSDL()
    {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
	    std::cerr << "Couldn't initialize SDL: " << SDL_GetError() << std::endl;
	    exit(1);
	}

	IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
    }

    void initWindow()
    {
	int windowFlags = 0;
	m_window = SDL_CreateWindow(
	    "Canvas",
	    SDL_WINDOWPOS_UNDEFINED,
	    SDL_WINDOWPOS_UNDEFINED,
	    k_windowWidth,
	    k_windowHeight,
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
	    SDL_UpdateTexture(m_texture, nullptr, m_pixels, k_windowWidth * sizeof(Uint32));

	    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
	    SDL_RenderClear(m_renderer);
	    SDL_Event event;
	    while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
		    exit(0);
		    break;
		case SDL_MOUSEBUTTONUP:
		    if (event.button.button == SDL_BUTTON_LEFT) {
			m_leftMouseButtonDown = false;
		    }
		case SDL_MOUSEBUTTONDOWN:
		    if (event.button.button == SDL_BUTTON_LEFT) {
			m_leftMouseButtonDown = true;
		    }
		case SDL_MOUSEMOTION:
		    if (m_leftMouseButtonDown) {
			int mouseX = event.motion.x;
			int mouseY = event.motion.y;
			m_pixels[mouseY * k_windowWidth + mouseX] = 0;
		    }
		}
	    }

	    SDL_RenderCopy(m_renderer, m_texture, nullptr, nullptr);

	    SDL_RenderPresent(m_renderer);
	    SDL_Delay(16);
	}
    }
};

int main(int argc, char** argv) {
    App app;
    return 0;
}
