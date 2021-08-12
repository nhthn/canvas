#include <iostream>

#include <SDL2/SDL.h>

#include <sdlgui/screen.h>
#include <sdlgui/window.h>
#include <sdlgui/layout.h>
#include <sdlgui/label.h>
#include <sdlgui/checkbox.h>
#include <sdlgui/button.h>
#include <sdlgui/toolbutton.h>
#include <sdlgui/popupbutton.h>
#include <sdlgui/combobox.h>
#include <sdlgui/dropdownbox.h>
#include <sdlgui/progressbar.h>
#include <sdlgui/entypo.h>
#include <sdlgui/messagedialog.h>
#include <sdlgui/textbox.h>
#include <sdlgui/slider.h>
#include <sdlgui/imagepanel.h>
#include <sdlgui/imageview.h>
#include <sdlgui/vscrollpanel.h>
#include <sdlgui/colorwheel.h>
#include <sdlgui/graph.h>
#include <sdlgui/tabwidget.h>
#include <sdlgui/switchbox.h>
#include <sdlgui/formhelper.h>
#include <memory>

#include "Synth.hpp"
#include "PortAudioBackend.hpp"
#include "RingBuffer.hpp"

constexpr int k_windowWidth = 2 * 640;
constexpr int k_windowHeight = 2 * 480;

constexpr int k_imageWidth = 640;
constexpr int k_imageHeight = 256;

int nextPowerOfTwo(int x) {
    int power = 1;
    while (power < x) {
	power *= 2;
    }
    return power;
}

class GUI : public sdlgui::Screen
{
public:
    GUI(SDL_Window* pwindow, int rwidth, int rheight)
      : sdlgui::Screen(pwindow, sdlgui::Vector2i(rwidth, rheight), "Canvas")
    {
        {
            auto& nwindow = window("Button demo", sdlgui::Vector2i{15, 15})
                .withLayout<sdlgui::GroupLayout>();

            nwindow.label("Push buttons", "sans-bold")._and()
                .button("Plain button", [] { std::cout << "pushed!" << std::endl; })
                .withTooltip("This is plain button tips");

            nwindow.button("Styled", ENTYPO_ICON_ROCKET, [] { std::cout << "pushed!" << std::endl; })
                .withBackgroundColor(sdlgui::Color(0, 0, 255, 25));
        }

        performLayout(mSDL_Renderer);
    }

    virtual bool keyboardEvent(int key, int scancode, int action, int modifiers)
    {
        return sdlgui::Screen::keyboardEvent(key, scancode, action, modifiers);
    }

    virtual void draw(SDL_Renderer* renderer)
    {
        sdlgui::Screen::draw(renderer);
    }

    virtual void drawContents()
    {
    }
};

class App {
public:
    App()
	: m_ringBuffer(
	    std::make_shared<RingBuffer<float>>(
		nextPowerOfTwo(k_windowHeight)
	    )
	)
	, m_synth(m_ringBuffer)
	, m_audioBackend(&m_synth)
    {
	initSDL();
	initWindow();
	initRenderer();
	initGUI();

	m_texture = SDL_CreateTexture(
	    m_renderer,
	    SDL_PIXELFORMAT_ARGB8888,
	    SDL_TEXTUREACCESS_STATIC,
	    k_imageWidth,
	    k_imageHeight
	);

	m_pixels = new Uint32[k_imageHeight * k_imageWidth];
	for (int i = 0; i < k_imageHeight * k_imageWidth; i++) {
	    m_pixels[i] = 0;
	}
    }

    ~App()
    {
	delete m_pixels;
    }

    void run()
    {
	m_audioBackend.run();
	mainLoop();
    }

private:
    std::shared_ptr<RingBuffer<float>> m_ringBuffer;
    Synth m_synth;
    PortAudioBackend m_audioBackend;

    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    SDL_Texture* m_texture;
    Uint32* m_pixels;
    std::unique_ptr<GUI> m_gui;

    bool m_leftMouseButtonDown;
    int m_lastMouseX = -1;
    int m_lastMouseY = -1;

    float m_position;
    float m_speedInPixelsPerSecond = 100;

    float m_overallGain = 0.05;

    void initSDL()
    {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
	    std::cerr << "Couldn't initialize SDL: " << SDL_GetError() << std::endl;
	    exit(1);
	}
    }

    void initWindow()
    {
	int windowFlags = SDL_WINDOW_ALLOW_HIGHDPI;
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

    void initGUI()
    {
	m_gui = std::make_unique<GUI>(m_window, k_windowWidth, k_windowHeight);
    }

    void drawPixel(int x, int y)
    {
	if (!((0 <= y) && (y < k_imageHeight))) {
	    return;
	}
	if (!((0 <= x) && (x < k_imageWidth))) {
	    return;
	}
	m_pixels[y * k_imageWidth + x] = 0xffffffff;
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

    void handleEvents() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
	    m_gui->onEvent(event);
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
		    int mouseX = (
			static_cast<float>(event.motion.x)
			* k_imageWidth / k_windowWidth
		    );
		    int mouseY = (
			static_cast<float>(event.motion.y)
			* k_imageHeight / k_windowHeight
		    );
		    if (m_lastMouseX >= 0 && m_lastMouseY >= 0) {
			drawLine(m_lastMouseX, m_lastMouseY, mouseX, mouseY, 5);
		    }
		    m_lastMouseX = mouseX;
		    m_lastMouseY = mouseY;
		}
		break;
	    }
	}
    }

    void mainLoop()
    {
	while (true) {
	    sendAmplitudesToAudioThread();
	    SDL_UpdateTexture(m_texture, nullptr, m_pixels, k_imageWidth * sizeof(Uint32));
	    handleEvents();

	    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
	    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
	    SDL_RenderClear(m_renderer);

	    SDL_RenderCopy(m_renderer, m_texture, nullptr, nullptr);

	    SDL_Rect fillRect = { static_cast<int>(static_cast<float>(m_position) * k_windowWidth / k_imageWidth), 0, 2, k_windowHeight };
	    SDL_SetRenderDrawColor(m_renderer, 0xff, 0xff, 0xff, 0xff);
	    SDL_RenderFillRect(m_renderer, &fillRect);

	    m_gui->drawAll();

	    SDL_RenderPresent(m_renderer);

	    int delayInMilliseconds = 16;
	    SDL_Delay(delayInMilliseconds);

	    m_position += m_speedInPixelsPerSecond * delayInMilliseconds / 1000;
	    while (m_position > k_imageWidth) {
		m_position -= k_imageWidth;
	    }
	}
    }

    void sendAmplitudesToAudioThread()
    {
	int position = m_position;
	float amplitudes[k_imageHeight];
	for (int i = 0; i < k_imageHeight; i++) {
	    amplitudes[i] = ((m_pixels[k_imageWidth * (k_imageHeight - 1 - i) + position] & 0x0000ff00) >> 8) / 255.0 * m_overallGain;
	}

	m_ringBuffer->write(amplitudes, k_imageHeight);
    }
};

int main(int argc, char** argv) {
    App app;
    app.run();
    return 0;
}
