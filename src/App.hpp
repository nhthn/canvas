#include <iostream>
#include <memory>
#include <random>

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

#include "Synth.hpp"
#include "PortAudioBackend.hpp"
#include "RingBuffer.hpp"

constexpr int k_windowWidth = 2 * 640;
constexpr int k_windowHeight = 2 * 480;

constexpr int k_imageWidth = 640;
constexpr int k_imageHeight = 256;

class GUI;

class App {
public:
    App();
    ~App();

    void run();

    enum class Mode {
        Draw,
        Erase,
        HorizontalLine
    };


    void setMode(Mode mode) { m_mode = mode; }
    Mode getMode() { return m_mode; }

    void clear();
    void applyScaleFilter(int root, int scaleClass);
    void applyReverb(float decay, float damping);
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

    Mode m_mode = Mode::Draw;

    void initSDL();
    void initWindow();
    void initRenderer();
    void initGUI();
    void mainLoop();
    void drawPixel(int x, int y, int color);
    void stampCircle(int x, int y, int radius, int color);
    void drawLine(int x1, int y1, int x2, int y2, int radius, int color);
    void handleEvents();
    void handleEventDrawOrErase(SDL_Event& event);
    void handleEventHorizontalLine(SDL_Event& event);
    void sendAmplitudesToAudioThread();
};

class SliderTextBox {
public:
    SliderTextBox(
        sdlgui::Widget& parent,
        float value,
        std::string label
    );

    float value();

private:
    float m_defaultValue;
    std::unique_ptr<sdlgui::Slider> m_slider;
    std::unique_ptr<sdlgui::TextBox> m_textBox;
};

class GUI : public sdlgui::Screen
{
public:
    GUI(App* app, SDL_Window* pwindow, int width, int height);

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

private:
    App* m_app;
    sdlgui::Button* m_drawButton;
    sdlgui::Button* m_eraseButton;
    sdlgui::Button* m_scaleFilterButton;

    std::unique_ptr<sdlgui::DropdownBox> m_scaleFilterRootDropDown;
    std::unique_ptr<sdlgui::DropdownBox> m_scaleFilterScaleClassDropDown;

    std::unique_ptr<SliderTextBox> m_decayWidget;
    std::unique_ptr<SliderTextBox> m_dampingWidget;

    std::unique_ptr<SliderTextBox> m_chorusRate;
    std::unique_ptr<SliderTextBox> m_chorusDepth;

    std::unique_ptr<SliderTextBox> m_tremoloRate;
    std::unique_ptr<SliderTextBox> m_tremoloDepth;
    std::unique_ptr<sdlgui::DropdownBox> m_tremoloShape;
    std::unique_ptr<SliderTextBox> m_tremoloStereo;

    std::unique_ptr<SliderTextBox> m_harmonics2;
    std::unique_ptr<SliderTextBox> m_harmonics3;
    std::unique_ptr<SliderTextBox> m_harmonics4;
    std::unique_ptr<SliderTextBox> m_harmonics5;
    std::unique_ptr<sdlgui::CheckBox> m_subharmonics;
};
