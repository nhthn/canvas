#include "App.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


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

void App::displayError(std::string message)
{
    m_gui->displayError(message);
}

bool App::loadAudio(std::string fileName)
{
    SF_INFO sf_info;
    sf_info.format = 0;
    auto soundFile = sf_open(fileName.c_str(), SFM_READ, &sf_info);

    if (soundFile == nullptr) {
        displayError(std::string("Audio loading failed: ") + sf_strerror(soundFile));
        return false;
    }

    if (sf_info.channels != 2) {
        displayError("File must have 2 channels");
        sf_close(soundFile);
        return false;
    }

    int frames = sf_info.frames;
    float* audio = new float[sf_info.frames * sf_info.channels];
    sf_read_float(soundFile, audio, sf_info.frames);
    sf_close(soundFile);

    int fftBufferSize = 2048;
    int spectrumSize = fftBufferSize / 2 + 1;
    float* fftInBuffer = new float[fftBufferSize];
    auto fftOutBuffer = static_cast<fftwf_complex*>(
        fftwf_malloc(sizeof(fftwf_complex) * spectrumSize)
    );
    float* magnitudeSpectrum = new float[spectrumSize];
    float* imageTmp = new float[k_imageHeight * k_imageWidth];

    auto fftwPlan = fftwf_plan_dft_r2c_1d(
        fftBufferSize, fftInBuffer, fftOutBuffer, FFTW_MEASURE
    );

    for (int i = 0; i < fftBufferSize; i++) {
        float window = 0.5 - 0.5 * std::cos(i * 2 * 3.141592653589 / fftBufferSize);
        fftInBuffer[i] = audio[i * 2] * window;
    }
    fftwf_execute(fftwPlan);

    float binToFreq = (sf_info.samplerate * 0.5f) / spectrumSize;
    float freqToBin = 1 / binToFreq;
    for (int y = 0; y < k_imageHeight; y++) {
        float minFreq = 27.5 * std::pow(2, (k_imageHeight - 1 - y - 1) / 24);
        float maxFreq = 27.5 * std::pow(2, (k_imageHeight - 1 - y + 1) / 24);
        int minBin = clamp<int>(static_cast<int>(freqToBin * minFreq), 0, spectrumSize - 1);
        int maxBin = clamp<int>(static_cast<int>(freqToBin * maxFreq), 0, spectrumSize - 1);
        float maxAmplitude = 0;
        for (int bin = minBin; bin <= maxBin; bin++) {
            float binAmplitude = std::hypot(fftOutBuffer[bin][0], fftOutBuffer[bin][1]);
            if (binAmplitude > maxAmplitude) {
                maxAmplitude = binAmplitude;
            }
        }
        for (int x = 0; x < k_imageHeight; x++) {
            imageTmp[y * k_imageWidth + x] = maxAmplitude;
        }
    };

    float overallMaxAmplitude = 0;
    for (int i = 0; i < k_imageHeight * k_imageWidth; i++) {
        if (imageTmp[i] > overallMaxAmplitude) {
            overallMaxAmplitude = imageTmp[i];
        }
    }
    if (overallMaxAmplitude != 0) {
        for (int i = 0; i < k_imageHeight * k_imageWidth; i++) {
            imageTmp[i] /= overallMaxAmplitude;
        }
    }
    for (int i = 0; i < k_imageHeight * k_imageWidth; i++) {
        m_pixels[i] = colorFromNormalized(imageTmp[i], imageTmp[i], imageTmp[i]);
    }

    fftwf_free(fftOutBuffer);
    delete[] imageTmp;
    delete[] magnitudeSpectrum;
    delete[] fftInBuffer;
    delete[] audio;

    return true;
}

bool App::renderAudio(std::string fileName)
{
    if (m_speedInPixelsPerSecond < 0.01) {
        displayError("Speed is too slow to render audio.");
        return false;
    }

    if (!endsWith(fileName, ".wav")) {
        displayError("File name must end in .wav");
        return false;
    }

    float sampleRate = m_audioBackend.getSampleRate();

    SF_INFO sf_info;
    sf_info.samplerate = sampleRate;
    sf_info.channels = 2;
    sf_info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
    sf_info.sections = 0;
    sf_info.seekable = 0;
    auto soundFile = sf_open(fileName.c_str(), SFM_WRITE, &sf_info);

    if (soundFile == nullptr) {
        displayError(std::string("Audio rendering failed: ") + sf_strerror(soundFile));
        return false;
    }

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
            int color = m_pixels[k_imageWidth * (k_imageHeight - 1 - i) + position];
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
            if (sampleOffset + i >= numFrames) {
                break;
            }
            audio[(sampleOffset + i) * 2] = inBuffer[0][i];
            audio[(sampleOffset + i) * 2 + 1] = inBuffer[1][i];
        }
        sampleOffset += blockSize;
    }

    sf_write_float(soundFile, audio, numFrames * 2);
    sf_close(soundFile);

    delete[] audio;
    delete[] leftInBuffer;
    delete[] rightInBuffer;
    delete[] leftOutBuffer;
    delete[] rightOutBuffer;

    return true;
}

bool App::loadImage(std::string fileName)
{
    int width;
    int height;
    int unused;
    int channels = 3;
    unsigned char* imageData = stbi_load(
        fileName.c_str(), &width, &height, &unused, channels
    );
    if (imageData == nullptr) {
        displayError(std::string("Image loading failed: ") + stbi_failure_reason());
        return false;
    }

    for (int i = 0; i < k_imageHeight; i++) {
        for (int j = 0; j < k_imageWidth; j++) {
            int x = static_cast<float>(j) * width / k_imageWidth;
            int y = static_cast<float>(i) * height / k_imageHeight;
            int offset = (y * width + x) * channels;
            unsigned char red = imageData[offset];
            unsigned char green = imageData[offset + 1];
            unsigned char blue = imageData[offset + 2];
            int color = 0xff000000 + (red << 16) + (green << 8) + blue;
            m_pixels[i * k_imageWidth + j] = color;
        }
    }

    stbi_image_free(imageData);

    return true;
}

bool App::saveImage(std::string fileName)
{
    int channels = 4;

    if (!endsWith(fileName, ".png")) {
        displayError("File name must end in .png");
        return false;
    }

    char* imageData = new char[k_imageHeight * k_imageWidth * channels];

    for (int i = 0; i < k_imageHeight; i++) {
        for (int j = 0; j < k_imageWidth; j++) {
            int color = m_pixels[i * k_imageWidth + j];
            int offset = (i * k_imageWidth + j) * channels;
            imageData[offset + 0] = getRed(color);
            imageData[offset + 1] = getGreen(color);
            imageData[offset + 2] = getBlue(color);
            imageData[offset + 3] = 255;
        }
    }

    int strideInBytes = k_imageWidth * channels;

    int success = stbi_write_png(
        fileName.c_str(), k_imageWidth, k_imageHeight, channels, imageData, strideInBytes
    );

    if (success == 0) {
        displayError("Image saving failed");
    }

    delete[] imageData;

    return true;
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
