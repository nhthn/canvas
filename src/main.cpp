#include <iostream>
#include <SDL2/SDL.h>

#include "audio.hpp"
#include "portaudio_backend.hpp"

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
	for (int i = 0; i < k_windowHeight * k_windowWidth; i++) {
	    m_pixels[i] = 0;
	}

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
    int m_lastMouseX = -1;
    int m_lastMouseY = -1;

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

    void drawPixel(int x, int y)
    {
	if (!((0 <= y) && (y < k_windowHeight))) {
	    return;
	}
	if (!((0 <= x) && (x < k_windowWidth))) {
	    return;
	}
	m_pixels[y * k_windowWidth + x] = 0xffffffff;
    }

    void stampCircle(int x, int y, int radius)
    {
	for (int dx = -radius; dx <= radius; dx++) {
	    for (int dy = -radius; dy <= radius; dy++) {
		if (dx * dx + dy * dy <= radius * radius) {
		    drawPixel(x + dx, y + dy);
		}
	    }
	}
    }

    void drawLine(int x1, int y1, int x2, int y2, int radius)
    {
	int dx = x2 - x1;
	int dy = y2 - y1;
	if (dx == 0 && dy == 0) {
	    stampCircle(x1, y1, radius);
	    return;
	}
	if (std::abs(dx) > std::abs(dy)) {
	    for (int x = std::min(x1, x2); x <= std::max(x1, x2); x++) {
		float y = y1 + std::round(static_cast<float>(x - x1) * dy / dx);
		stampCircle(x, static_cast<int>(y), radius);
	    }
	} else {
	    for (int y = std::min(y1, y2); y <= std::max(y1, y2); y++) {
		float x = x1 + std::round(static_cast<float>(y - y1) * dx / dy);
		stampCircle(static_cast<int>(x), y, radius);
	    }
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
		    break;
		case SDL_MOUSEBUTTONDOWN:
		    if (event.button.button == SDL_BUTTON_LEFT) {
			m_leftMouseButtonDown = true;
			m_lastMouseX = -1;
			m_lastMouseY = -1;
		    }
		    break;
		case SDL_MOUSEMOTION:
		    if (m_leftMouseButtonDown) {
			int mouseX = event.motion.x;
			int mouseY = event.motion.y;
			if (m_lastMouseX >= 0 && m_lastMouseY >= 0) {
			    drawLine(m_lastMouseX, m_lastMouseY, mouseX, mouseY, 5);
			}
			m_lastMouseX = mouseX;
			m_lastMouseY = mouseY;
		    }
		    break;
		}
	    }

	    SDL_RenderCopy(m_renderer, m_texture, nullptr, nullptr);

	    SDL_RenderPresent(m_renderer);
	    SDL_Delay(16);
	}
    }
};

int main(int argc, char** argv) {
    World world;
    PortAudioBackend backend(&world);
    backend.run();
    backend.end();
    App app;
    return 0;
}
