#include "App.hpp"

#include "io.hpp"


App::App()
    : m_ringBuffer(
        std::make_shared<RingBuffer<float>>(
            nextPowerOfTwo(2 + k_windowHeight * 2)
        )
    )
    , m_randomEngine(m_randomDevice())
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
    clear();
}

App::~App()
{
    delete[] m_pixels;
}

void App::initAudio()
{
    m_audioBackend.run();

    float sampleRate = m_audioBackend.getSampleRate();

    m_synth = std::make_unique<Synth>(sampleRate);
    m_audioBackend.setCallback([this](
        int outChannels,
        float** output_buffer,
        int numFrames
    ) {
        m_synth->processRealtime(
            outChannels, output_buffer, numFrames, m_ringBuffer
        );
    });
}

void App::run()
{
    initAudio();
    mainLoop();
}

void App::startPlayback()
{
    m_playing = true;
}

void App::stopPlayback()
{
    m_position = 0;
    m_playing = false;
}

void App::displayError(std::string message)
{
    m_gui->displayError(message);
}

void App::clear()
{
    Image image(m_pixels, k_imageWidth, k_imageHeight);
    filters::clear(image);
}

void App::applyInvert()
{
    Image image(m_pixels, k_imageWidth, k_imageHeight);
    filters::applyInvert(image);
}

void App::applyReverb(float decay, float damping, bool reverse)
{
    Image image(m_pixels, k_imageWidth, k_imageHeight);
    filters::applyReverb(image, decay, damping, reverse);
}

void App::applyChorus(float rate, float depth)
{
    Image image(m_pixels, k_imageWidth, k_imageHeight);
    filters::applyChorus(image, rate, depth);
}


void App::applyScaleFilter(int root, int scaleClass)
{
    Image image(m_pixels, k_imageWidth, k_imageHeight);
    filters::applyScaleFilter(image, root, scaleClass);
}

void App::applyTremolo(float rate, float depth, int shape, float stereo)
{
    Image image(m_pixels, k_imageWidth, k_imageHeight);
    filters::applyTremolo(image, rate, depth, shape, stereo);
}

void App::applyHarmonics(
    float amplitude2,
    float amplitude3,
    float amplitude4,
    float amplitude5,
    bool subharmonics
)
{
    Image image(m_pixels, k_imageWidth, k_imageHeight);
    filters::applyHarmonics(
        image, amplitude2, amplitude3, amplitude4, amplitude5, subharmonics
    );
}

bool App::loadAudio(std::string fileName) {
    Image image(m_pixels, k_imageWidth, k_imageHeight);
    auto status = io::loadAudio(image, fileName);
    bool success = std::get<0>(status);
    std::string errorMessage = std::get<1>(status);
    if (!success) {
        displayError(errorMessage);
    }
    return success;
}

bool App::renderAudio(std::string fileName) {
    Image image(m_pixels, k_imageWidth, k_imageHeight);
    auto status = io::renderAudio(
        image,
        fileName,
        m_audioBackend.getSampleRate(),
        m_overallGain,
        m_speedInPixelsPerSecond,
        m_pdMode,
        m_pdDistort
    );
    bool success = std::get<0>(status);
    std::string errorMessage = std::get<1>(status);
    if (!success) {
        displayError(errorMessage);
    }
    return success;
}

bool App::loadImage(std::string fileName) {
    Image image(m_pixels, k_imageWidth, k_imageHeight);
    auto status = io::loadImage(image, fileName);
    bool success = std::get<0>(status);
    std::string errorMessage = std::get<1>(status);
    if (!success) {
        displayError(errorMessage);
    }
    return success;
}

bool App::saveImage(std::string fileName) {
    Image image(m_pixels, k_imageWidth, k_imageHeight);
    auto status = io::saveImage(image, fileName);
    bool success = std::get<0>(status);
    std::string errorMessage = std::get<1>(status);
    if (!success) {
        displayError(errorMessage);
    }
    return success;
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

void App::drawPixel(int x, int y, float red, float green, float blue, float alpha)
{
    Image image(m_pixels, k_imageWidth, k_imageHeight);
    draw::drawPixel(image, x, y, red, green, blue, alpha);
}

void App::drawFuzzyCircle(int x, int y, int radius, float red, float green, float blue, float alpha)
{
    Image image(m_pixels, k_imageWidth, k_imageHeight);
    draw::drawFuzzyCircle(image, x, y, radius, red, green, blue, alpha);
}

void App::drawLine(int x1, int y1, int x2, int y2, int radius, float red, float green, float blue, float alpha)
{
    Image image(m_pixels, k_imageWidth, k_imageHeight);
    draw::drawLine(image, x1, y1, x2, y2, radius, red, green, blue, alpha);
}

void App::spray(
    int x,
    int y,
    float radius,
    float density,
    float red,
    float green,
    float blue,
    float alpha
)
{
    Image image(m_pixels, k_imageWidth, k_imageHeight);
    draw::spray(image, x, y, radius, density, red, green, blue, alpha, m_randomEngine);
}

void App::sprayLine(int x1, int y1, int x2, int y2, int radius, float density, float red, float green, float blue, float alpha)
{
    Image image(m_pixels, k_imageWidth, k_imageHeight);
    draw::sprayLine(image, x1, y1, x2, y2, radius, density, red, green, blue, alpha, m_randomEngine);
}

void App::handleEventDrawEraseAndSpray(SDL_Event& event)
{
    int toolbarWidth = m_gui->getWindowWidth();
    int radius = m_brushSize / 2;

    float red = m_mode == App::Mode::Erase ? 0 : m_red;
    float green = m_mode == App::Mode::Erase ? 0 : m_green;
    float blue = m_mode == App::Mode::Erase ? 0 : m_blue;
    float alpha = m_mode == App::Mode::Erase ? 1 : m_opacity;

    switch (event.type) {
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
                static_cast<float>(event.motion.x - toolbarWidth)
                * k_imageWidth / (k_windowWidth - toolbarWidth)
            );
            int mouseY = (
                static_cast<float>(event.motion.y)
                * k_imageHeight / k_windowHeight
            );
            if (m_mode == App::Mode::Spray) {
                spray(
                    mouseX, mouseY, radius, m_sprayDensity, red, green, blue, alpha
                );
            } else {
                drawFuzzyCircle(
                    mouseX, mouseY, radius, red, green, blue, alpha
                );
            }
        }
        break;
    case SDL_MOUSEMOTION:
        if (m_leftMouseButtonDown) {
            int mouseX = (
                static_cast<float>(event.motion.x - toolbarWidth)
                * k_imageWidth / (k_windowWidth - toolbarWidth)
            );
            int mouseY = (
                static_cast<float>(event.motion.y)
                * k_imageHeight / k_windowHeight
            );
            if (m_lastMouseX >= 0 && m_lastMouseY >= 0) {
                if (m_mode == App::Mode::Spray) {
                    sprayLine(
                        m_lastMouseX,
                        m_lastMouseY,
                        mouseX,
                        mouseY,
                        radius,
                        m_sprayDensity,
                        red,
                        green,
                        blue,
                        alpha
                    );
                } else {
                    drawLine(
                        m_lastMouseX,
                        m_lastMouseY,
                        mouseX,
                        mouseY,
                        radius,
                        red,
                        green,
                        blue,
                        alpha
                    );
                }
            }
            m_lastMouseX = mouseX;
            m_lastMouseY = mouseY;
        }
        break;
    }
}

void App::handleEventHorizontalLine(SDL_Event& event) {
    if (event.type != SDL_MOUSEBUTTONDOWN) {
        return;
    }
    if (event.button.button == SDL_BUTTON_LEFT) {
        int mouseY = (
            static_cast<float>(event.motion.y)
            * k_imageHeight / k_windowHeight
        );
        for (int x = 0; x < k_imageWidth; x++) {
            drawPixel(x, mouseY, m_red, m_green, m_blue, m_opacity);
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
        if (event.type == SDL_QUIT) {
            exit(0);
        }
        if (
            m_mode == App::Mode::Draw
            || m_mode == App::Mode::Erase
            || m_mode == App::Mode::Spray
        ) {
            handleEventDrawEraseAndSpray(event);
        }
        if (m_mode == App::Mode::HorizontalLine) {
            handleEventHorizontalLine(event);
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

        int toolbarWidth = m_gui->getWindowWidth();
        SDL_Rect imageRect = {
            toolbarWidth,
            0,
            k_windowWidth - toolbarWidth,
            k_windowHeight
        };
        SDL_RenderCopy(m_renderer, m_texture, nullptr, &imageRect);

        if (m_playing) {
            SDL_Rect fillRect = {
                toolbarWidth + static_cast<int>(
                    static_cast<float>(m_position)
                    * (k_windowWidth - toolbarWidth) / k_imageWidth
                ),
                0,
                2,
                k_windowHeight
            };
            SDL_SetRenderDrawColor(m_renderer, 0xff, 0xff, 0xff, 0xff);
            SDL_RenderFillRect(m_renderer, &fillRect);
        }

        m_gui->drawAll();

        SDL_RenderPresent(m_renderer);

        int delayInMilliseconds = 16;
        SDL_Delay(delayInMilliseconds);

        if (m_playing) {
            m_position += m_speedInPixelsPerSecond * delayInMilliseconds / 1000;
            while (m_position > k_imageWidth) {
                m_position -= k_imageWidth;
            }
        }
    }
}

void App::sendAmplitudesToAudioThread()
{
    int position = m_position;

    constexpr int amplitudeOffset = 2;
    constexpr int size = amplitudeOffset + 2 * k_imageHeight;
    float data[size];

    data[0] = m_pdMode;
    data[1] = m_pdDistort;

    if (!m_playing) {
        for (int i = 0; i < 2 * k_imageHeight; i++) {
            data[amplitudeOffset + i] = 0;
        }
    } else {
        for (int i = 0; i < k_imageHeight; i++) {
            int index = k_imageWidth * (k_imageHeight - 1 - i) + position;
            data[amplitudeOffset + 2 * i] = (m_pixels[index] & 0x000000ff) / 255.0 * m_overallGain;
            data[amplitudeOffset + 2 * i + 1] = ((m_pixels[index] & 0x00ff0000) >> 16) / 255.0 * m_overallGain;
        }
    }

    m_ringBuffer->write(data, 2 + 2 * k_imageHeight);
}
