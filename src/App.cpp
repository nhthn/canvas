#include "App.hpp"

static int nextPowerOfTwo(int x) {
    int power = 1;
    while (power < x) {
	power *= 2;
    }
    return power;
}

static int getRed(int color) {
    return (color & 0xff0000) >> 16;
}

static float getRedNormalized(int color) {
    return getRed(color) / 255.f;
}

static int getGreen(int color) {
    return (color & 0x00ff00) >> 8;
}

static float getGreenNormalized(int color) {
    return getGreen(color) / 255.f;
}

static int getBlue(int color) {
    return ((color & 0x0000ff) >> 0);
}

static float getBlueNormalized(int color) {
    return getBlue(color) / 255.f;
}

static uint32_t colorFromNormalized(float red, float green, float blue)
{
    return (
        0xff000000
        + (static_cast<int>(red * 255) << 16)
        + (static_cast<int>(green * 255) << 8)
        + (static_cast<int>(blue * 255) << 0)
    );
}

SliderTextBox::SliderTextBox(
    sdlgui::Widget& parent,
    float value,
    std::string label
)
    : m_defaultValue(value)
{
    auto& widget = parent
        .widget()
        .withLayout<sdlgui::BoxLayout>(
            sdlgui::Orientation::Horizontal, sdlgui::Alignment::Middle, 0, 20
        );

    widget.label(label)
        .withFixedSize(sdlgui::Vector2i(60, 25));

    m_slider = std::make_unique<sdlgui::Slider>(
        &widget,
        value,
        [this](sdlgui::Slider* slider, float value) {
            m_textBox->setValue(std::to_string((int)(value * 100)));
        }
    );
    m_slider->withFixedWidth(80);

    m_textBox = std::make_unique<sdlgui::TextBox>(
        &widget,
        std::to_string(static_cast<int>(value * 100)),
        "%"
    );
    m_textBox
        ->withAlignment(sdlgui::TextBox::Alignment::Right)
        .withFixedSize(sdlgui::Vector2i(60, 25))
        .withFontSize(20);
}

float SliderTextBox::value() {
    return m_slider->value();
}

GUI::GUI(App* app, SDL_Window* pwindow, int width, int height)
    : m_app(app)
    , sdlgui::Screen(pwindow, sdlgui::Vector2i(width, height), "Canvas")
{
    {
        auto& nwindow = window("Toolbox", sdlgui::Vector2i{15, 15})
            .withLayout<sdlgui::GroupLayout>();

        m_drawButton = &nwindow.button("Draw", ENTYPO_ICON_PENCIL, [this] {
            m_app->setMode(App::Mode::Draw);
        }).withFlags(sdlgui::Button::RadioButton);
        m_drawButton->setPushed(true);

        nwindow.button("Erase", ENTYPO_ICON_ERASE, [this] {
            m_app->setMode(App::Mode::Erase);
        }).withFlags(sdlgui::Button::RadioButton);

        nwindow.button("— Horiz. Line", [this] {
            m_app->setMode(App::Mode::HorizontalLine);
        }).withFlags(sdlgui::Button::RadioButton);

        nwindow.label("Filters");

        nwindow.button("Clear", [this] {
            m_app->clear();
        });

        ////////////////

        auto& scaleFilterPopup = nwindow.popupbutton("Scale Filter")
            .popup()
            .withLayout<sdlgui::GroupLayout>();

        m_scaleFilterRootDropDown = std::make_unique<sdlgui::DropdownBox>(
            &scaleFilterPopup,
            std::vector<std::string> {
                "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
            }
        );

        m_scaleFilterScaleClassDropDown = std::make_unique<sdlgui::DropdownBox>(
            &scaleFilterPopup,
            std::vector<std::string> {
                "Major",
                "Minor",
                "Acoustic",
                "Harmonic Major",
                "Harmonic Minor",
                "Whole Tone",
                "Octatonic",
                "Hexatonic (Messiaen)"
            }
        );
        m_scaleFilterScaleClassDropDown->withFixedWidth(240);

        scaleFilterPopup.button("Apply", [this] {
            m_app->applyScaleFilter(
                m_scaleFilterRootDropDown->selectedIndex(),
                m_scaleFilterScaleClassDropDown->selectedIndex()
            );
        });

        ////////////////

        auto& reverbPopup = nwindow.popupbutton("Reverb")
            .popup()
            .withLayout<sdlgui::GroupLayout>();

        m_decayWidget = std::make_unique<SliderTextBox>(reverbPopup, 0.5f, "Decay");

        m_dampingWidget = std::make_unique<SliderTextBox>(reverbPopup, 0.5f, "Damping");

        reverbPopup.button("Apply", [this] {
            m_app->applyReverb(
                m_decayWidget->value(),
                m_dampingWidget->value()
            );
        });

        ////////////////

        auto& chorusPopup = nwindow.popupbutton("Chorus")
            .popup()
            .withLayout<sdlgui::GroupLayout>();

        m_chorusRate = std::make_unique<SliderTextBox>(chorusPopup, 0.5, "Rate");

        m_chorusDepth = std::make_unique<SliderTextBox>(chorusPopup, 0.8, "Depth");

        chorusPopup.button("Apply", [this] {
            m_app->applyChorus(
                m_chorusRate->value(),
                m_chorusDepth->value()
            );
        });

        ////////////////

        auto& tremoloPopup = nwindow.popupbutton("Tremolo")
            .popup()
            .withLayout<sdlgui::GroupLayout>();

        m_tremoloRate = std::make_unique<SliderTextBox>(tremoloPopup, 0.5, "Rate");

        m_tremoloDepth = std::make_unique<SliderTextBox>(tremoloPopup, 1.0, "Depth");

        m_tremoloStereo = std::make_unique<SliderTextBox>(tremoloPopup, 0.0, "Stereo");

        m_tremoloShape = std::make_unique<sdlgui::DropdownBox>(
            &tremoloPopup,
            std::vector<std::string> {
                "Sine",
                "Triangle",
                "Square",
                "Saw Down",
                "Saw Up"
            }
        );

        tremoloPopup.button("Apply", [this] {
            m_app->applyTremolo(
                m_tremoloRate->value(),
                m_tremoloDepth->value(),
                m_tremoloShape->selectedIndex(),
                m_tremoloStereo->value()
            );
        });
    }

    performLayout(mSDL_Renderer);
}

App::App()
    : m_ringBuffer(
        std::make_shared<RingBuffer<float>>(
            nextPowerOfTwo(k_windowHeight * 2)
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
    clear();
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

void App::clear()
{
    for (int i = 0; i < k_imageHeight * k_imageWidth; i++) {
        m_pixels[i] = 0xff000000;
    }
}

void App::applyReverb(float decay, float damping)
{
    float baseDecayLength = 1 + (decay * k_imageWidth * 2);
    for (int row = 0; row < k_imageHeight; row++) {
        float decayLength = (
            baseDecayLength * std::pow(static_cast<float>(row) / k_imageHeight, damping)
        );
        float k = std::pow(0.001f, 1.0f / decayLength);
        float lastRed = 0;
        float lastGreen = 0;
        float lastBlue = 0;
        for (int column = 0; column < k_imageWidth; column++) {
            int index = row * k_imageWidth + column;
            int color = m_pixels[index];
            float red = getRedNormalized(color);
            float green = getGreenNormalized(color);
            float blue = getBlueNormalized(color);
            red = std::max(lastRed * k, red);
            green = std::max(lastGreen * k, green);
            blue = std::max(lastBlue * k, blue);
            m_pixels[index] = colorFromNormalized(red, green, blue);
            lastRed = red;
            lastGreen = green;
            lastBlue = blue;
        }
    }
}


class RandomLFO {
public:
    RandomLFO(std::mt19937& rng, int period)
        : m_rng(rng), m_period(period), m_distribution(0.0, 1.0)
    {
        m_current = m_distribution(rng);
        m_target = m_distribution(rng);
        m_t = 0;
    }

    float process()
    {
        float value = (
            m_current * static_cast<float>(m_period - m_t) / m_period
            + m_target * static_cast<float>(m_t) / m_period
        );
        m_t += 1;
        if (m_t >= m_period) {
            m_t = 0;
            m_current = m_target;
            m_target = m_distribution(m_rng);
        }
        return value;
    }

private:
    std::mt19937 m_rng;
    int m_period;
    std::uniform_real_distribution<> m_distribution;
    float m_current;
    float m_target;
    int m_t;
};


void App::applyChorus(float rate, float depth)
{
    for (int row = 0; row < k_imageHeight; row++) {
        std::random_device randomDevice;
        std::mt19937 rng(randomDevice());
        int lfoPeriod = 1000.f / (k_imageHeight - 1 - row) / (0.05 + rate);
        RandomLFO lfoRed(rng, lfoPeriod);
        RandomLFO lfoGreen(rng, lfoPeriod);
        RandomLFO lfoBlue(rng, lfoPeriod);
        for (int column = 0; column < k_imageWidth; column++) {
            int index = row * k_imageWidth + column;
            int color = m_pixels[index];
            float red = getRedNormalized(color);
            float green = getGreenNormalized(color);
            float blue = getBlueNormalized(color);

            red *= 1 - lfoRed.process() * depth;
            green *= 1 - lfoGreen.process() * depth;
            blue *= 1 - lfoBlue.process() * depth;

            color = colorFromNormalized(red, green, blue);
            m_pixels[index] = color;
        }
    }
}


void App::applyScaleFilter(int root, int scaleClass)
{
    int scale[][12] = {
        { 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1 }, // Major
        { 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0 }, // Minor
        { 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0 }, // Acoustic
        { 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1 }, // Harmonic Major
        { 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1 }, // Harmonic Minor
        { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 }, // Whole Tone
        { 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1 }, // Octatonic
        { 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1 }, // Hexatonic (Messiaen)
    };
    for (int row = 0; row < k_imageHeight; row++) {
        // Subtract 6 because lowest frequency is A.
        int stepsIn24EDO = k_imageHeight - 1 - row - 6;
        int offsetFromRoot = ((stepsIn24EDO / 2 - root) % 12 + 12) % 12;
        if (stepsIn24EDO % 2 != 0 || scale[scaleClass][offsetFromRoot] == 0) {
            for (int column = 0; column < k_imageWidth; column++) {
                m_pixels[row * k_imageWidth + column] = 0;
            }
        }
    }
}

static float tremoloLFO(float phase, int shape)
{
    switch (shape) {
    case 0: // Sine
        return std::cos(phase * 3.14159265358979 * 2) * 0.5 + 0.5;
    case 1: // Triangle
        return phase >= 0.5 ? 2 - 2 * phase : 2 * phase;
    case 2: // Square
        return phase < 0.5;
    case 3: // Saw Down -- squaring the signal sounds better
        return (1 - phase) * (1 - phase);
    case 4: // Saw Up
        return phase;
    }
    return 0;
}

void App::applyTremolo(float rate, float depth, int shape, float stereo)
{
    for (int row = 0; row < k_imageHeight; row++) {
        float lfoPhase = 0;
        float lfoPeriod = std::pow(k_imageWidth, 1 - rate);
        float phaseIncrement = 1 / lfoPeriod;
        for (int column = 0; column < k_imageWidth; column++) {
            float lfo1 = tremoloLFO(lfoPhase, shape);
            float lfo2 = tremoloLFO(lfoPhase + 0.5 * stereo, shape);
            lfoPhase += phaseIncrement;
            while (lfoPhase > 1.0) {
                lfoPhase -= 1.0;
            }

            int index = row * k_imageWidth + column;
            int color = m_pixels[index];
            float red = getRedNormalized(color);
            float green = getGreenNormalized(color);
            float blue = getBlueNormalized(color);

            red *= 1 - (1 - lfo1) * depth;
            green *= 1 - (1 - (lfo1 + lfo2) * 0.5) * depth;
            blue *= 1 - (1 - lfo2) * depth;

            color = colorFromNormalized(red, green, blue);
            m_pixels[index] = color;
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

void App::handleEventDrawOrErase(SDL_Event& event) {
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
                m_mode == App::Mode::Erase ? 0xff000000 : 0xffff00ff
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
                    m_mode == App::Mode::Erase ? 0xff000000 : 0xffff00ff
                );
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
        for (int i = 0; i < k_imageWidth; i++) {
            m_pixels[mouseY * k_imageWidth + i] = 0xffff00ff;
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
        if (m_mode == App::Mode::Draw || m_mode == App::Mode::Erase) {
            handleEventDrawOrErase(event);
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
    float amplitudes[2 * k_imageHeight];
    for (int i = 0; i < k_imageHeight; i++) {
        int index = k_imageWidth * (k_imageHeight - 1 - i) + position;
        amplitudes[2 * i] = (m_pixels[index] & 0x000000ff) / 255.0 * m_overallGain;
        amplitudes[2 * i + 1] = ((m_pixels[index] & 0x00ff0000) >> 16) / 255.0 * m_overallGain;
    }

    m_ringBuffer->write(amplitudes, 2 * k_imageHeight);
}
