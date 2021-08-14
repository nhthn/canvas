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

float clamp01(float x) {
    return std::max(std::min(x, 1.0f), 0.0f);
}

static uint32_t colorFromNormalized(float red, float green, float blue)
{
    return (
        0xff000000
        + (static_cast<int>(clamp01(red) * 255) << 16)
        + (static_cast<int>(clamp01(green) * 255) << 8)
        + (static_cast<int>(clamp01(blue) * 255) << 0)
    );
}

SliderTextBox::SliderTextBox(
    sdlgui::Widget& parent,
    float value,
    std::string label,
    std::function<void(float)> onChange
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
        [this, onChange](sdlgui::Slider* slider, float value) {
            onChange(value);
            m_textBox->setValue(std::to_string((int)(value * 100)));
        }
    );
    m_slider->withFixedWidth(80);

    onChange(value);

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
    auto& nwindow = window("Toolbox", sdlgui::Vector2i{0, 0});
    nwindow.setDraggable(false);

    nwindow.withLayout<sdlgui::GroupLayout>();

    m_drawButton = &nwindow.button("Draw", ENTYPO_ICON_PENCIL, [this] {
        m_app->setMode(App::Mode::Draw);
    }).withFlags(sdlgui::Button::RadioButton);
    m_drawButton->setPushed(true);

    nwindow.button("Erase", [this] {
        m_app->setMode(App::Mode::Erase);
    }).withFlags(sdlgui::Button::RadioButton);

    nwindow.button("Spray", [this] {
        m_app->setMode(App::Mode::Spray);
    }).withFlags(sdlgui::Button::RadioButton);

    nwindow.button("— Horiz. Line", [this] {
        m_app->setMode(App::Mode::HorizontalLine);
    }).withFlags(sdlgui::Button::RadioButton);

    ////////////////

    m_brushSize = std::make_unique<SliderTextBox>(
        nwindow, 0.1f, "Size", [this](float normalizedSize) {
            float brushSize = 1 + 99 * normalizedSize;
            m_app->setBrushSize(brushSize);
        }
    );
    m_colorRed = std::make_unique<SliderTextBox>(
        nwindow, 1.0f, "Red", [this](float red) {
            m_app->setRed(red);
        }
    );
    m_colorGreen = std::make_unique<SliderTextBox>(
        nwindow, 0.0f, "Green", [this](float green) {
            m_app->setGreen(green);
        }
    );
    m_colorBlue = std::make_unique<SliderTextBox>(
        nwindow, 1.0f, "Blue", [this](float blue) {
            m_app->setBlue(blue);
        }
    );
    m_colorOpacity = std::make_unique<SliderTextBox>(
        nwindow, 1.0f, "Opacity", [this](float opacity) {
            m_app->setOpacity(opacity);
        }
    );

    ////////////////

    nwindow.label("Playback");

    auto& transport = nwindow.widget().withLayout<sdlgui::BoxLayout>(
        sdlgui::Orientation::Horizontal, sdlgui::Alignment::Middle, 0, 5
    );

    auto& playButton = transport.button("", ENTYPO_ICON_PLAY, [this] {
        m_app->startPlayback();
    }).withBackgroundColor(sdlgui::Color(0, 255, 0, 25));
    playButton.setFixedSize(sdlgui::Vector2i { 25, 25 });

    auto& stopButton = transport.button("", ENTYPO_ICON_STOP, [this] {
        m_app->stopPlayback();
    }).withBackgroundColor(sdlgui::Color(255, 0, 0, 25));
    stopButton.setFixedSize(sdlgui::Vector2i { 25, 25 });

    transport.label("Speed");

    transport.slider(0.5, [this] (sdlgui::Slider* slider, float value) {
        m_app->setSpeedInPixelsPerSecond(value * 200);
    });

    ////////////////

    nwindow.label("Synth");

    m_pdMode = std::make_unique<sdlgui::DropdownBox>(
        &nwindow,
        std::vector<std::string> { "Pulsar PD", "Saw PD" }
    );
    m_pdMode->setCallback([this](int pdMode) {
        m_app->setPDMode(pdMode);
    });

    m_pdDistort = std::make_unique<SliderTextBox>(
        nwindow, 0.0f, "Distort", [this](float pdDistort) {
            m_app->setPDDistort(pdDistort);
        }
    );

    ////////////////

    nwindow.label("File");

    nwindow.button("Render audio", [this] {
        m_app->renderAudio("");
    });

    ////////////////

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

    m_reverbDecay = std::make_unique<SliderTextBox>(reverbPopup, 0.5f, "Decay");
    m_reverbDamping = std::make_unique<SliderTextBox>(reverbPopup, 0.5f, "Damping");
    m_reverbReverse = std::make_unique<sdlgui::CheckBox>(&reverbPopup, "Reverse");

    reverbPopup.button("Apply", [this] {
        m_app->applyReverb(
            m_reverbDecay->value(),
            m_reverbDamping->value(),
            m_reverbReverse->checked()
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

    ////////////////

    auto& harmonicsPopup = nwindow.popupbutton("Harmonics")
        .popup()
        .withLayout<sdlgui::GroupLayout>();

    m_harmonics2 = std::make_unique<SliderTextBox>(harmonicsPopup, 1.0/2, "2");
    m_harmonics3 = std::make_unique<SliderTextBox>(harmonicsPopup, 1.0/3, "3");
    m_harmonics4 = std::make_unique<SliderTextBox>(harmonicsPopup, 1.0/4, "4");
    m_harmonics5 = std::make_unique<SliderTextBox>(harmonicsPopup, 1.0/5, "5");
    m_subharmonics = std::make_unique<sdlgui::CheckBox>(&harmonicsPopup, "Subharmonics");

    harmonicsPopup.button("Apply", [this] {
        m_app->applyHarmonics(
            m_harmonics2->value(),
            m_harmonics3->value(),
            m_harmonics4->value(),
            m_harmonics5->value(),
            m_subharmonics->checked()
        );
    });

    performLayout(mSDL_Renderer);

    nwindow.setHeight(height);

    m_windowWidth = nwindow.width();
}

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
    delete m_pixels;
}

void App::initAudio()
{
    m_audioBackend.run();

    float sampleRate = m_audioBackend.getSampleRate();

    m_synth = std::make_unique<Synth>(sampleRate);
    m_audioBackend.setCallback([this](
        int inChannels,
        int outChannels,
        const float** input_buffer,
        float** output_buffer,
        int numFrames
    ) {
        m_synth->processRealtime(
            inChannels, outChannels, input_buffer, output_buffer, numFrames, m_ringBuffer
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

void App::renderAudio(std::string fileName)
{
    float sampleRate = m_audioBackend.getSampleRate();
    int numFrames = (
        static_cast<float>(k_imageWidth) / m_speedInPixelsPerSecond * sampleRate
    );
    float* audio = new float[numFrames * 2];

    Synth synth(sampleRate);
    synth.setPDMode(m_pdMode);
    synth.setPDDistort(m_pdDistort);

    int inChannels = 2;
    int outChannels = 2;
    int blockSize = 64;

    float* leftInBuffer = new float[blockSize];
    float* rightInBuffer = new float[blockSize];
    const float* inBuffer[2] = { leftInBuffer, rightInBuffer };

    float* leftOutBuffer = new float[blockSize];
    float* rightOutBuffer = new float[blockSize];
    float* outBuffer[2] = { leftInBuffer, rightInBuffer };

    int sampleOffset = 0;
    while (sampleOffset <= numFrames) {
        int position = static_cast<float>(sampleOffset) * k_imageWidth / numFrames;
        for (int i = 0; i < k_imageHeight; i++) {
            int color = m_pixels[i * k_imageWidth + position];
            synth.setOscillatorAmplitude(
                i,
                getBlueNormalized(color) * m_overallGain,
                getRedNormalized(color) * m_overallGain
            );
        }
        synth.process(
            inChannels, outChannels, inBuffer, outBuffer, blockSize
        );
        for (int i = 0; i < blockSize; i++) {
            audio[(sampleOffset + i) * 2] = inBuffer[0][i];
            audio[(sampleOffset + i) * 2 + 1] = inBuffer[1][i];
        }
        sampleOffset += blockSize;
    }

    SF_INFO sf_info;
    sf_info.samplerate = sampleRate;
    sf_info.channels = 1;
    sf_info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
    sf_info.sections = 0;
    sf_info.seekable = 0;
    auto soundFile = sf_open("/home/nathan/out.wav", SFM_WRITE, &sf_info);

    sf_write_float(soundFile, audio, numFrames * 2);
    sf_close(soundFile);

    delete[] audio;
    delete[] leftInBuffer;
    delete[] rightInBuffer;
    delete[] leftOutBuffer;
    delete[] rightOutBuffer;
}

void App::clear()
{
    for (int i = 0; i < k_imageHeight * k_imageWidth; i++) {
        m_pixels[i] = 0xff000000;
    }
}

void App::applyReverb(float decay, float damping, bool reverse)
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
            int index = (
                row * k_imageWidth
                + (reverse ? k_imageWidth - 1 - column : column)
            );
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

void App::applyHarmonics(
    float amplitude2,
    float amplitude3,
    float amplitude4,
    float amplitude5,
    bool subharmonics
)
{
    for (int row = 0; row < k_imageHeight; row++) {
        for (int column = 0; column < k_imageWidth; column++) {
            float red2 = 0, green2 = 0, blue2 = 0;
            float red3 = 0, green3 = 0, blue3 = 0;
            float red4 = 0, green4 = 0, blue4 = 0;
            float red5 = 0, green5 = 0, blue5 = 0;

            int sign = subharmonics ? -1 : 1;

            int index2 = (row + 12 * 2 * sign) * k_imageWidth + column;
            if (0 <= index2 && index2 < k_imageWidth * k_imageHeight) {
                int color2 = m_pixels[index2];
                red2 = getRedNormalized(color2);
                green2 = getGreenNormalized(color2);
                blue2 = getBlueNormalized(color2);
            }

            int index3 = (row + (12 + 7) * 2 * sign) * k_imageWidth + column;
            if (0 <= index3 && index3 < k_imageWidth * k_imageHeight) {
                int color3 = m_pixels[index3];
                red3 = getRedNormalized(color3);
                green3 = getGreenNormalized(color3);
                blue3 = getBlueNormalized(color3);
            }

            int index4 = (row + 24 * 2 * sign) * k_imageWidth + column;
            if (0 <= index4 && index4 < k_imageWidth * k_imageHeight) {
                int color4 = m_pixels[index4];
                red4 = getRedNormalized(color4);
                green4 = getGreenNormalized(color4);
                blue4 = getBlueNormalized(color4);
            }

            int index5 = (row + (24 + 4) * 2 * sign) * k_imageWidth + column;
            if (0 <= index5 && index5 < k_imageWidth * k_imageHeight) {
                int color5 = m_pixels[index5];
                red5 = getRedNormalized(color5);
                green5 = getGreenNormalized(color5);
                blue5 = getBlueNormalized(color5);
            }

            int index = row * k_imageWidth + column;
            int color = m_pixels[index];
            float red = getRedNormalized(color);
            float green = getGreenNormalized(color);
            float blue = getBlueNormalized(color);

            red += (
                red2 * amplitude2
                + red3 * amplitude3
                + red4 * amplitude4
                + red5 * amplitude5
            );
            green += (
                green2 * amplitude2
                + green3 * amplitude3
                + green4 * amplitude4
                + green5 * amplitude5
            );
            blue += (
                blue2 * amplitude2
                + blue3 * amplitude3
                + blue4 * amplitude4
                + blue5 * amplitude5
            );

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

void App::drawPixel(int x, int y, float red, float green, float blue, float alpha)
{
    if (!((0 <= y) && (y < k_imageHeight))) {
        return;
    }
    if (!((0 <= x) && (x < k_imageWidth))) {
        return;
    }
    int index = y * k_imageWidth + x;
    int color = m_pixels[index];
    float newRed = getRedNormalized(color);
    float newGreen = getGreenNormalized(color);
    float newBlue = getBlueNormalized(color);
    newRed = newRed * (1 - alpha) + red * alpha;
    newGreen = newGreen * (1 - alpha) + green * alpha;
    newBlue = newBlue * (1 - alpha) + blue * alpha;
    m_pixels[index] = colorFromNormalized(newRed, newGreen, newBlue);
}

void App::drawFuzzyCircle(int x, int y, int radius, float red, float green, float blue, float alpha)
{
    for (int dx = -radius; dx <= radius; dx++) {
        for (int dy = -radius; dy <= radius; dy++) {
            if (dx * dx + dy * dy <= radius * radius) {
                float pixelRadius = std::sqrt(dx * dx + dy * dy);
                float pixelAlpha = alpha * (1 - pixelRadius / radius);
                drawPixel(x + dx, y + dy, red, green, blue, pixelAlpha);
            }
        }
    }
}

void App::drawLine(int x1, int y1, int x2, int y2, int radius, float red, float green, float blue, float alpha)
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    if (dx == 0 && dy == 0) {
        drawFuzzyCircle(x1, y1, radius, red, green, blue, alpha);
        return;
    }
    if (std::abs(dx) > std::abs(dy)) {
        for (int x = std::min(x1, x2); x <= std::max(x1, x2); x++) {
            float y = y1 + std::round(static_cast<float>(x - x1) * dy / dx);
            drawFuzzyCircle(x, static_cast<int>(y), radius, red, green, blue, alpha);
        }
    } else {
        for (int y = std::min(y1, y2); y <= std::max(y1, y2); y++) {
            float x = x1 + std::round(static_cast<float>(y - y1) * dx / dy);
            drawFuzzyCircle(static_cast<int>(x), y, radius, red, green, blue, alpha);
        }
    }
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
    std::uniform_real_distribution<> bipolar(-1.0, 1.0);
    std::uniform_real_distribution<> unipolar(0.0, 1.0);
    for (int i = 0; i < radius * radius * density; i++) {
        float dx = radius * (
            bipolar(m_randomEngine)
            + bipolar(m_randomEngine)
            + bipolar(m_randomEngine)
        ) / 3.0f;
        float dy = radius * (
            bipolar(m_randomEngine)
            + bipolar(m_randomEngine)
            + bipolar(m_randomEngine)
        ) / 3.0f;
        float pixelAlpha = alpha * unipolar(m_randomEngine);
        drawPixel(x + dx, y + dy, red, green, blue, pixelAlpha);
    }
}

void App::sprayLine(int x1, int y1, int x2, int y2, int radius, float density, float red, float green, float blue, float alpha)
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    if (dx == 0 && dy == 0) {
        spray(x1, y1, radius, density, red, green, blue, alpha);
        return;
    }
    if (std::abs(dx) > std::abs(dy)) {
        for (int x = std::min(x1, x2); x <= std::max(x1, x2); x++) {
            float y = y1 + std::round(static_cast<float>(x - x1) * dy / dx);
            spray(x, static_cast<int>(y), radius, density, red, green, blue, alpha);
        }
    } else {
        for (int y = std::min(y1, y2); y <= std::max(y1, y2); y++) {
            float x = x1 + std::round(static_cast<float>(y - y1) * dx / dy);
            spray(static_cast<int>(x), y, radius, density, red, green, blue, alpha);
        }
    }
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

    int amplitudeOffset = 2;
    int size = amplitudeOffset + 2 * k_imageHeight;
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
