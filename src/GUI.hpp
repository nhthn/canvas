#pragma once
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

#include "App.hpp"

class App;

class SliderTextBox {
public:
    SliderTextBox(
        sdlgui::Widget& parent,
        float value,
        std::string label,
        std::function<void(float)> onChange = [] (float value) { }
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

    int getWindowWidth() { return m_windowWidth; }

private:
    App* m_app;
    int m_windowWidth;

    sdlgui::Button* m_drawButton;

    std::unique_ptr<SliderTextBox> m_brushSize;
    std::unique_ptr<SliderTextBox> m_colorRed;
    std::unique_ptr<SliderTextBox> m_colorGreen;
    std::unique_ptr<SliderTextBox> m_colorBlue;
    std::unique_ptr<SliderTextBox> m_colorOpacity;

    std::unique_ptr<sdlgui::DropdownBox> m_pdMode;
    std::unique_ptr<SliderTextBox> m_pdDistort;

    std::unique_ptr<sdlgui::TextBox> m_loadImagePath;

    std::unique_ptr<sdlgui::DropdownBox> m_scaleFilterRootDropDown;
    std::unique_ptr<sdlgui::DropdownBox> m_scaleFilterScaleClassDropDown;

    std::unique_ptr<SliderTextBox> m_reverbDecay;
    std::unique_ptr<SliderTextBox> m_reverbDamping;
    std::unique_ptr<sdlgui::CheckBox> m_reverbReverse;

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
