#include "GUI.hpp"

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
    nwindow.setDropShadowEnabled(false);

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
        std::vector<std::string> { "Pulsar PD", "Saw PD", "Square PD", "Sine PWM" }
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

    auto& renderAudioButton = nwindow.popupbutton("Render Audio");
    auto& renderAudioPopup = renderAudioButton.popup().withLayout<sdlgui::GroupLayout>();

    m_renderAudioPath = std::make_unique<sdlgui::TextBox>(
        &renderAudioPopup,
        getHomeDirectory() + getPathSeparator() + "out.wav"
    );
    m_renderAudioPath->withAlignment(sdlgui::TextBox::Alignment::Left);
    m_renderAudioPath->setEditable(true);

    renderAudioPopup.button("Render", [this, &renderAudioButton] {
        bool success = m_app->renderAudio(m_renderAudioPath->value());
        if (success) {
            renderAudioButton.setPushed(false);
        }
    });

    auto& loadImageButton = nwindow.popupbutton("Load Image");
    auto& loadImagePopup = loadImageButton.popup().withLayout<sdlgui::GroupLayout>();

    m_loadImagePath = std::make_unique<sdlgui::TextBox>(
        &loadImagePopup,
        getHomeDirectory() + getPathSeparator() + "in.png"
    );
    m_loadImagePath->withAlignment(sdlgui::TextBox::Alignment::Left);
    m_loadImagePath->setEditable(true);

    loadImagePopup.button("Load", [this, &loadImageButton] {
        bool success = m_app->loadImage(m_loadImagePath->value());
        if (success) {
            loadImageButton.setPushed(false);
        }
    });

    auto& saveImageButton = nwindow.popupbutton("Save Image");
    auto& saveImagePopup = saveImageButton.popup().withLayout<sdlgui::GroupLayout>();

    m_saveImagePath = std::make_unique<sdlgui::TextBox>(
        &saveImagePopup,
        getHomeDirectory() + getPathSeparator() + "out.png"
    );
    m_loadImagePath->withAlignment(sdlgui::TextBox::Alignment::Left);
    m_loadImagePath->setEditable(true);

    saveImagePopup.button("Save", [this, &saveImageButton] {
        bool success = m_app->saveImage(m_saveImagePath->value());
        if (success) {
            saveImageButton.setPushed(false);
        }
    });

    ////////////////

    nwindow.label("Filters");

    nwindow.button("Clear", [this] {
        m_app->clear();
    });

    nwindow.button("Invert", [this] {
        m_app->applyInvert();
    });

    ////////////////

    auto& scaleFilterButton = nwindow.popupbutton("Scale Filter");
    auto& scaleFilterPopup = scaleFilterButton.popup().withLayout<sdlgui::GroupLayout>();

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

    scaleFilterPopup.button("Apply", [this, &scaleFilterButton] {
        m_app->applyScaleFilter(
            m_scaleFilterRootDropDown->selectedIndex(),
            m_scaleFilterScaleClassDropDown->selectedIndex()
        );
        scaleFilterButton.setPushed(false);
    });

    ////////////////

    auto& reverbButton = nwindow.popupbutton("Reverb");
    auto& reverbPopup = reverbButton.popup().withLayout<sdlgui::GroupLayout>();

    m_reverbDecay = std::make_unique<SliderTextBox>(reverbPopup, 0.5f, "Decay");
    m_reverbDamping = std::make_unique<SliderTextBox>(reverbPopup, 0.5f, "Damping");
    m_reverbReverse = std::make_unique<sdlgui::CheckBox>(&reverbPopup, "Reverse");

    reverbPopup.button("Apply", [this, &reverbButton] {
        m_app->applyReverb(
            m_reverbDecay->value(),
            m_reverbDamping->value(),
            m_reverbReverse->checked()
        );
        reverbButton.setPushed(false);
    });

    ////////////////

    auto& chorusButton = nwindow.popupbutton("Chorus");
    auto& chorusPopup = chorusButton.popup().withLayout<sdlgui::GroupLayout>();

    m_chorusRate = std::make_unique<SliderTextBox>(chorusPopup, 0.5, "Rate");
    m_chorusDepth = std::make_unique<SliderTextBox>(chorusPopup, 0.8, "Depth");

    chorusPopup.button("Apply", [this, &chorusButton] {
        m_app->applyChorus(
            m_chorusRate->value(),
            m_chorusDepth->value()
        );
        chorusButton.setPushed(false);
    });

    ////////////////

    auto& tremoloButton = nwindow.popupbutton("Tremolo");
    auto& tremoloPopup = tremoloButton.popup().withLayout<sdlgui::GroupLayout>();

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

    tremoloPopup.button("Apply", [this, &tremoloButton] {
        m_app->applyTremolo(
            m_tremoloRate->value(),
            m_tremoloDepth->value(),
            m_tremoloShape->selectedIndex(),
            m_tremoloStereo->value()
        );
        tremoloButton.setPushed(false);
    });

    ////////////////

    auto& harmonicsButton = nwindow.popupbutton("Harmonics");
    auto& harmonicsPopup = harmonicsButton.popup().withLayout<sdlgui::GroupLayout>();

    m_harmonics2 = std::make_unique<SliderTextBox>(harmonicsPopup, 1.0/2, "2");
    m_harmonics3 = std::make_unique<SliderTextBox>(harmonicsPopup, 1.0/3, "3");
    m_harmonics4 = std::make_unique<SliderTextBox>(harmonicsPopup, 1.0/4, "4");
    m_harmonics5 = std::make_unique<SliderTextBox>(harmonicsPopup, 1.0/5, "5");
    m_subharmonics = std::make_unique<sdlgui::CheckBox>(&harmonicsPopup, "Subharmonics");

    harmonicsPopup.button("Apply", [this, &harmonicsButton] {
        m_app->applyHarmonics(
            m_harmonics2->value(),
            m_harmonics3->value(),
            m_harmonics4->value(),
            m_harmonics5->value(),
            m_subharmonics->checked()
        );
        harmonicsButton.setPushed(false);
    });

    ////////////////

    performLayout(mSDL_Renderer);

    nwindow.setHeight(height);

    m_windowWidth = nwindow.width();
}

void GUI::displayError(std::string message)
{
    msgdialog(sdlgui::MessageDialog::Type::Warning, "Error", message);
}
