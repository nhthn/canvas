#pragma once
#include <iostream>
#include <memory>
#include <random>

#include <SDL2/SDL.h>
#include <sndfile.h>

#include "stb_image.h"

#include "common.hpp"
#include "GUI.hpp"
#include "Synth.hpp"
#include "PortAudioBackend.hpp"
#include "RingBuffer.hpp"

constexpr int k_windowWidth = 2 * 640;
constexpr int k_windowHeight = 2 * 480;

constexpr int k_imageWidth = 640;
constexpr int k_imageHeight = 239;

class GUI;

class App {
public:
    App();
    ~App();

    void run();

    enum class Mode {
        Draw,
        Erase,
        Spray,
        HorizontalLine
    };

    void setMode(Mode mode) { m_mode = mode; }
    Mode getMode() { return m_mode; }

    void setRed(float red) { m_red = red; }
    void setGreen(float green) { m_green = green; }
    void setBlue(float blue) { m_blue = blue; }
    void setOpacity(float opacity) { m_opacity = opacity; }
    void setBrushSize(float brushSize) { m_brushSize = brushSize; }

    void renderAudio(std::string fileName);
    void loadImage(std::string fileName);

    void startPlayback();
    void stopPlayback();
    void setSpeedInPixelsPerSecond(float speedInPixelsPerSecond) {
        m_speedInPixelsPerSecond = speedInPixelsPerSecond;
    };

    void setPDMode(int pdMode) { m_pdMode = pdMode; };
    void setPDDistort(float pdDistort) { m_pdDistort = pdDistort; };

    void clear();
    void applyScaleFilter(int root, int scaleClass);
    void applyReverb(float decay, float damping, bool reverse);
    void applyChorus(float rate, float depth);
    void applyTremolo(float rate, float depth, int shape, float stereo);
    void applyHarmonics(
        float amplitude2,
        float amplitude3,
        float amplitude4,
        float amplitude5,
        bool subharmonics
    );

private:
    std::shared_ptr<RingBuffer<float>> m_ringBuffer;
    std::unique_ptr<Synth> m_synth;
    PortAudioBackend m_audioBackend;

    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    SDL_Texture* m_texture;
    Uint32* m_pixels;
    std::unique_ptr<GUI> m_gui;

    bool m_leftMouseButtonDown = false;
    int m_lastMouseX = -1;
    int m_lastMouseY = -1;

    bool m_playing = false;
    float m_position = 0;
    float m_speedInPixelsPerSecond = 100;

    float m_overallGain = 0.05;

    Mode m_mode = Mode::Draw;

    float m_sprayDensity = 0.1;

    float m_red;
    float m_green;
    float m_blue;
    float m_opacity;
    float m_brushSize;

    int m_pdMode = 0;
    float m_pdDistort = 0.0;

    std::random_device m_randomDevice;
    std::mt19937 m_randomEngine;

    void initSDL();
    void initWindow();
    void initRenderer();
    void initGUI();
    void initAudio();
    void mainLoop();
    void drawPixel(int x, int y, float red, float green, float blue, float alpha);
    void drawFuzzyCircle(int x, int y, int radius, float red, float green, float blue, float alpha);
    void drawLine(int x1, int y1, int x2, int y2, int radius, float red, float green, float blue, float alpha);
    void spray(int x, int y, float radius, float density, float red, float green, float blue, float alpha);
    void sprayLine(int x1, int y1, int x2, int y2, int radius, float density, float red, float green, float blue, float alpha);
    void handleEvents();
    void handleEventDrawEraseAndSpray(SDL_Event& event);
    void handleEventHorizontalLine(SDL_Event& event);
    void sendAmplitudesToAudioThread();
};
