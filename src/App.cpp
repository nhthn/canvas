#include "App.hpp"

static int nextPowerOfTwo(int x) {
    int power = 1;
    while (power < x) {
	power *= 2;
    }
    return power;
}

GUI::GUI(App* app, SDL_Window* pwindow, int width, int height)
    : m_app(app)
    , sdlgui::Screen(pwindow, sdlgui::Vector2i(width, height), "Canvas")
{
    {
        auto& nwindow = window("Toolbox", sdlgui::Vector2i{15, 15})
            .withLayout<sdlgui::GroupLayout>();

        m_drawButton = &nwindow.button("Draw", [this] {
            m_app->setMode(App::Mode::Draw);
        }).withFlags(sdlgui::Button::RadioButton);
        m_drawButton->setPushed(true);

        m_eraseButton = &nwindow.button("Erase", [this] {
            m_app->setMode(App::Mode::Erase);
        }).withFlags(sdlgui::Button::RadioButton);

        nwindow.label("Filters");

        m_scaleFilterButton = &nwindow.button("Scale Filter", [this] {
            m_app->applyScaleFilter();
        });

        nwindow.button("Reverb", [this] {
            m_app->applyReverb();
        });
    }

    performLayout(mSDL_Renderer);
}

App::App()
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
        m_pixels[i] = 0xff000000;
    }
}

App::~App()
{
    delete m_pixels;
}

void App::run()
{
    m_audioBackend.run();
    mainLoop();
}

void App::applyReverb()
{
    float decayRate = 0.99;
    for (int row = 0; row < k_imageHeight; row++) {
        float lastRed = 0;
        float lastGreen = 0;
        float lastBlue = 0;
        for (int column = 0; column < k_imageWidth; column++) {
            int index = row * k_imageWidth + column;
            int color = m_pixels[index];
            float red = ((color & 0xff0000) >> 16) / 255.0;
            float green = ((color & 0x00ff00) >> 8) / 255.0;
            float blue = ((color & 0x0000ff) >> 0) / 255.0;
            red = std::max(lastRed * decayRate, red);
            green = std::max(lastGreen * decayRate, green);
            blue = std::max(lastBlue * decayRate, blue);
            color = (
                0xff000000
                + (static_cast<int>(red * 255) << 16)
                + (static_cast<int>(green * 255) << 8)
                + (static_cast<int>(blue * 255) << 0)
            );
            m_pixels[index] = color;
            lastRed = red;
            lastGreen = green;
            lastBlue = blue;
        }
    }
}

void App::applyScaleFilter()
{
    int scale[12] = { 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1 };
    for (int row = 0; row < k_imageHeight; row++) {
        if (row % 2 == 1 || scale[row / 2 % 12] == 0) {
            for (int column = 0; column < k_imageWidth; column++) {
                m_pixels[row * k_imageWidth + column] = 0;
            }
        }
    }
}

void App::initSDL()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Couldn't initialize SDL: " << SDL_GetError() << std::endl;
        exit(1);
    }
}

void App::initWindow()
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

void App::initRenderer()
{
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    int rendererFlags = SDL_RENDERER_ACCELERATED;
    m_renderer = SDL_CreateRenderer(m_window, -1, rendererFlags);
    if (m_renderer == nullptr) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        exit(1);
    }
}

void App::initGUI()
{
    m_gui = std::make_unique<GUI>(this, m_window, k_windowWidth, k_windowHeight);
}

void App::drawPixel(int x, int y, int color)
{
    if (!((0 <= y) && (y < k_imageHeight))) {
        return;
    }
    if (!((0 <= x) && (x < k_imageWidth))) {
        return;
    }
    m_pixels[y * k_imageWidth + x] = color;
}

void App::stampCircle(int x, int y, int radius, int color)
{
    for (int dx = -radius; dx <= radius; dx++) {
        for (int dy = -radius; dy <= radius; dy++) {
            if (dx * dx + dy * dy <= radius * radius) {
                drawPixel(x + dx, y + dy, color);
            }
        }
    }
}

void App::drawLine(int x1, int y1, int x2, int y2, int radius, int color)
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    if (dx == 0 && dy == 0) {
        stampCircle(x1, y1, radius, color);
        return;
    }
    if (std::abs(dx) > std::abs(dy)) {
        for (int x = std::min(x1, x2); x <= std::max(x1, x2); x++) {
            float y = y1 + std::round(static_cast<float>(x - x1) * dy / dx);
            stampCircle(x, static_cast<int>(y), radius, color);
        }
    } else {
        for (int y = std::min(y1, y2); y <= std::max(y1, y2); y++) {
            float x = x1 + std::round(static_cast<float>(y - y1) * dx / dy);
            stampCircle(static_cast<int>(x), y, radius, color);
        }
    }
}

void App::handleEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        bool guiProcessedEvent = m_gui->onEvent(event);
        if (guiProcessedEvent) {
            continue;
        }
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
                int mouseX = (
                    static_cast<float>(event.motion.x)
                    * k_imageWidth / k_windowWidth
                );
                int mouseY = (
                    static_cast<float>(event.motion.y)
                    * k_imageHeight / k_windowHeight
                );
                stampCircle(
                    mouseX,
                    mouseY,
                    5,
                    m_mode == App::Mode::Erase ? 0xff000000 : 0xffffffff
                );
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
                    drawLine(
                        m_lastMouseX,
                        m_lastMouseY,
                        mouseX,
                        mouseY,
                        5,
                        m_mode == App::Mode::Erase ? 0xff000000 : 0xffffffff
                    );
                }
                m_lastMouseX = mouseX;
                m_lastMouseY = mouseY;
            }
            break;
        }
    }
}

void App::mainLoop()
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

void App::sendAmplitudesToAudioThread()
{
    int position = m_position;
    float amplitudes[k_imageHeight];
    for (int i = 0; i < k_imageHeight; i++) {
        amplitudes[i] = ((m_pixels[k_imageWidth * (k_imageHeight - 1 - i) + position] & 0x0000ff00) >> 8) / 255.0 * m_overallGain;
    }

    m_ringBuffer->write(amplitudes, k_imageHeight);
}
