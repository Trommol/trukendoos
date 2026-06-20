#include "PluginEditor.h"
#include "BinaryData.h"

#include <map>

namespace
{
    constexpr auto ink = 0xff17130f;
    constexpr auto cream = 0xfffff1bd;
    constexpr auto coral = 0xffff5a45;
    constexpr auto hotPink = 0xffff3f7d;
    constexpr auto mint = 0xff62f4c8;
    constexpr auto teal = 0xff1cafa8;
    constexpr auto darkPanel = 0xff1a1713;
    constexpr auto metal = 0xffd8d0b7;

    juce::RangedAudioParameter* parameterFor(juce::AudioProcessorValueTreeState& state, const juce::String& id)
    {
        return state.getParameter(id);
    }

    void setRealParameterValue(juce::AudioProcessorValueTreeState& state, const juce::String& id, float value)
    {
        if (auto* parameter = parameterFor(state, id))
            parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    }

    float getRealParameterValue(juce::AudioProcessorValueTreeState& state, const juce::String& id, float fallback = 0.0f)
    {
        if (auto* parameter = state.getRawParameterValue(id))
            return parameter->load();

        return fallback;
    }

    juce::Colour modeAccentForIndex(int index)
    {
        switch (index)
        {
            case 1: return juce::Colour(coral);
            case 2: return juce::Colour(hotPink);
            default: return juce::Colour(mint);
        }
    }

    juce::Colour modeFillForIndex(int index)
    {
        switch (index)
        {
            case 1: return juce::Colour(0xff311712);
            case 2: return juce::Colour(0xff201526);
            default: return juce::Colour(0xff122b25);
        }
    }

    const juce::Image& loadAsset(const char* data, int size)
    {
        static std::map<const char*, juce::Image> cache;
        auto iter = cache.find(data);

        if (iter == cache.end())
            iter = cache.emplace(data, juce::ImageFileFormat::loadFrom(data, size)).first;

        return iter->second;
    }

    const juce::Image& backplateImage() { return loadAsset(BinaryData::backplate_png, BinaryData::backplate_pngSize); }
    const juce::Image& globalBusImage() { return loadAsset(BinaryData::global_bus_plate_png, BinaryData::global_bus_plate_pngSize); }
    const juce::Image& presetPlateImage() { return loadAsset(BinaryData::preset_plate_png, BinaryData::preset_plate_pngSize); }
    const juce::Image& inputBadgeImage() { return loadAsset(BinaryData::input_badge_png, BinaryData::input_badge_pngSize); }
    const juce::Image& outputBadgeImage() { return loadAsset(BinaryData::output_badge_png, BinaryData::output_badge_pngSize); }
    const juce::Image& harmonizerPedalImage() { return loadAsset(BinaryData::pedal_harmonizer_png, BinaryData::pedal_harmonizer_pngSize); }

    const juce::Image& headerImageForMode(int index)
    {
        switch (index)
        {
            case 1: return loadAsset(BinaryData::header_rust_png, BinaryData::header_rust_pngSize);
            case 2: return loadAsset(BinaryData::header_glass_png, BinaryData::header_glass_pngSize);
            default: return loadAsset(BinaryData::header_melt_png, BinaryData::header_melt_pngSize);
        }
    }

    const juce::Image& pedalImageForMode(int index)
    {
        switch (index)
        {
            case 1: return loadAsset(BinaryData::pedal_rust_png, BinaryData::pedal_rust_pngSize);
            case 2: return loadAsset(BinaryData::pedal_glass_png, BinaryData::pedal_glass_pngSize);
            default: return loadAsset(BinaryData::pedal_melt_png, BinaryData::pedal_melt_pngSize);
        }
    }

    const juce::Image& knobImageForSlider(const juce::Slider& slider)
    {
        const auto id = slider.getComponentID();

        if (id == "knob-rust")
            return loadAsset(BinaryData::knob_rust_png, BinaryData::knob_rust_pngSize);

        if (id == "knob-melt")
            return loadAsset(BinaryData::knob_melt_png, BinaryData::knob_melt_pngSize);

        if (id == "knob-glass")
            return loadAsset(BinaryData::knob_glass_png, BinaryData::knob_glass_pngSize);

        if (id == "knob-harmonizer")
            return loadAsset(BinaryData::knob_harmonizer_png, BinaryData::knob_harmonizer_pngSize);

        return loadAsset(BinaryData::knob_global_png, BinaryData::knob_global_pngSize);
    }

    void drawImageWithin(juce::Graphics& g, const juce::Image& image, juce::Rectangle<float> area, float alpha = 1.0f)
    {
        if (image.isValid())
        {
            g.saveState();
            g.setOpacity(alpha);
            g.drawImageWithin(image, static_cast<int>(area.getX()), static_cast<int>(area.getY()),
                              static_cast<int>(area.getWidth()), static_cast<int>(area.getHeight()),
                              juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
            g.restoreState();
        }
    }

    void drawImageToFill(juce::Graphics& g, const juce::Image& image, juce::Rectangle<float> area)
    {
        if (image.isValid())
            g.drawImageWithin(image, static_cast<int>(area.getX()), static_cast<int>(area.getY()),
                              static_cast<int>(area.getWidth()), static_cast<int>(area.getHeight()),
                              juce::RectanglePlacement::stretchToFit);
    }

    juce::Rectangle<float> modePedalBoundsForIndex(int index)
    {
        switch (index)
        {
            case 1: return { 506.0f, 183.0f, 461.0f, 569.0f };
            case 2: return { 506.0f, 183.0f, 460.0f, 573.0f };
            default: return { 506.0f, 183.0f, 460.0f, 577.0f };
        }
    }

    juce::Rectangle<float> scopeBoundsForModeIndex(int index)
    {
        switch (index)
        {
            case 1: return { 604.0f, 578.0f, 266.0f, 128.0f };
            case 2: return { 593.0f, 586.0f, 286.0f, 123.0f };
            default: return { 594.0f, 578.0f, 281.0f, 125.0f };
        }
    }
}

SpectralRotAudioProcessorEditor::RotLookAndFeel::RotLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(cream));
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff15110e));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff4a352b));
    setColour(juce::ComboBox::textColourId, juce::Colour(cream));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2d3838));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff0e0c0a));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff161512));
    setColour(juce::PopupMenu::textColourId, juce::Colour(cream));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(coral));
}

void SpectralRotAudioProcessorEditor::RotLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                                                       float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                                                       juce::Slider& slider)
{
    auto drawArea = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height));
    const auto side = juce::jmin(drawArea.getWidth(), drawArea.getHeight());
    auto bounds = juce::Rectangle<float>(side, side).withCentre(drawArea.getCentre());
    const auto radius = side * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto& knob = knobImageForSlider(slider);

    if (knob.isValid())
    {
        g.setColour(juce::Colour(0x66000000));
        g.fillEllipse(bounds.translated(4.0f, 6.0f).reduced(7.0f));

        auto solidBody = bounds.reduced(side * 0.11f);
        auto bodyColour = juce::Colour(0xff1d1713);

        if (slider.getComponentID() == "knob-melt")
            bodyColour = juce::Colour(0xff0d3a34);
        else if (slider.getComponentID() == "knob-glass")
            bodyColour = juce::Colour(0xff20182b);
        else if (slider.getComponentID() == "knob-global")
            bodyColour = juce::Colour(0xff4f4b43);
        else if (slider.getComponentID() == "knob-harmonizer")
            bodyColour = juce::Colour(0xffcdbf99);
        else if (slider.getComponentID() == "knob-rust")
            bodyColour = juce::Colour(0xff32160e);

        juce::ColourGradient bodyGradient(bodyColour.brighter(0.35f), solidBody.getX(), solidBody.getY(),
                                          bodyColour.darker(0.45f), solidBody.getRight(), solidBody.getBottom(), false);
        g.setGradientFill(bodyGradient);
        g.fillEllipse(solidBody);

        const auto scale = side / static_cast<float>(juce::jmax(knob.getWidth(), knob.getHeight()));
        auto transform = juce::AffineTransform::translation(-knob.getWidth() * 0.5f, -knob.getHeight() * 0.5f)
            .scaled(scale)
            .rotated(angle)
            .translated(centre.x, centre.y);

        g.drawImageTransformed(knob, transform, false);

        juce::ignoreUnused(angle);
        return;
    }

    g.setColour(juce::Colour(0x66000000));
    g.fillEllipse(bounds.translated(4.0f, 5.0f));

    juce::ColourGradient outer(juce::Colour(0xffff7a6a), centre.x - radius, centre.y - radius,
                               juce::Colour(0xff7f1f25), centre.x + radius, centre.y + radius, false);
    g.setGradientFill(outer);
    g.fillEllipse(bounds);

    auto cap = bounds.reduced(radius * 0.18f);
    juce::ColourGradient capGradient(juce::Colour(0xffffa094), cap.getX(), cap.getY(),
                                     juce::Colour(0xffb94640), cap.getRight(), cap.getBottom(), false);
    g.setGradientFill(capGradient);
    g.fillEllipse(cap);

    g.setColour(juce::Colour(ink));
    g.drawEllipse(bounds, 2.2f);
    g.setColour(juce::Colour(0x55fff1bd));
    g.drawEllipse(cap.reduced(3.0f), 1.0f);

    auto pointer = juce::Path();
    pointer.addRoundedRectangle(-2.2f, -radius * 0.78f, 4.4f, radius * 0.38f, 2.0f);
    pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
    g.setColour(slider.isMouseOverOrDragging() ? juce::Colour(mint) : juce::Colour(cream));
    g.fillPath(pointer);

    juce::ignoreUnused(rotaryStartAngle, rotaryEndAngle);
}

void SpectralRotAudioProcessorEditor::RotLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                                                       bool shouldDrawButtonAsHighlighted,
                                                                       bool shouldDrawButtonAsDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
    const auto on = button.getToggleState();
    const auto light = on ? juce::Colour(coral) : juce::Colour(0xff403a33);

    g.setColour(juce::Colour(0x66000000));
    g.fillRoundedRectangle(bounds.translated(2.0f, 3.0f), 5.0f);
    g.setColour(on ? juce::Colour(0xff241613) : juce::Colour(0xff181614));
    g.fillRoundedRectangle(bounds, 5.0f);
    g.setColour(shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted ? juce::Colour(cream) : juce::Colour(0xff675a4c));
    g.drawRoundedRectangle(bounds, 5.0f, 1.2f);

    auto lamp = bounds.withWidth(16.0f).withHeight(16.0f).withCentre({ bounds.getX() + 12.0f, bounds.getCentreY() });
    g.setColour(light.withAlpha(on ? 0.85f : 0.45f));
    g.fillEllipse(lamp);
    g.setColour(juce::Colour(ink));
    g.drawEllipse(lamp, 1.0f);

    g.setColour(on ? juce::Colour(cream) : juce::Colour(0xff9a8a74));
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText(button.getButtonText(), button.getLocalBounds().withTrimmedLeft(24), juce::Justification::centredLeft);
}

void SpectralRotAudioProcessorEditor::RotLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                                                   int, int, int, int, juce::ComboBox&)
{
    auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)).reduced(1.0f);
    g.setColour(isButtonDown ? juce::Colour(0xff324646) : juce::Colour(0xff253434));
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(juce::Colour(0xff080706));
    g.drawRoundedRectangle(bounds, 4.0f, 1.4f);

    juce::Path arrow;
    arrow.startNewSubPath(bounds.getRight() - 18.0f, bounds.getCentreY() - 3.0f);
    arrow.lineTo(bounds.getRight() - 11.0f, bounds.getCentreY() + 4.0f);
    arrow.lineTo(bounds.getRight() - 4.0f, bounds.getCentreY() - 3.0f);
    g.setColour(juce::Colour(cream));
    g.strokePath(arrow, juce::PathStrokeType(2.0f));
}

juce::Font SpectralRotAudioProcessorEditor::RotLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return juce::FontOptions(14.0f, juce::Font::bold);
}

SpectralRotAudioProcessorEditor::SpectralRotAudioProcessorEditor(SpectralRotAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&rotLookAndFeel);
    auto& state = processor.getValueTreeState();

    auto configureModeBox = [] (juce::ComboBox& box)
    {
        box.addItem("Melt", 1);
        box.addItem("Rust", 2);
        box.addItem("Glass", 3);
    };

    addAndMakeVisible(modeBox);
    configureModeBox(modeBox);
    modeAttachment = std::make_unique<ComboBoxAttachment>(state, "mode", modeBox);

    addAndMakeVisible(slot1Box);
    addAndMakeVisible(slot2Box);
    addAndMakeVisible(slot3Box);
    configureModeBox(slot1Box);
    configureModeBox(slot2Box);
    configureModeBox(slot3Box);
    slot1Attachment = std::make_unique<ComboBoxAttachment>(state, "slot1Mode", slot1Box);
    slot2Attachment = std::make_unique<ComboBoxAttachment>(state, "slot2Mode", slot2Box);
    slot3Attachment = std::make_unique<ComboBoxAttachment>(state, "slot3Mode", slot3Box);
    slot1Box.setVisible(false);
    slot2Box.setVisible(false);
    slot3Box.setVisible(false);

    addAndMakeVisible(slot1Button);
    addAndMakeVisible(slot2Button);
    addAndMakeVisible(slot3Button);
    slot1Button.setButtonText("1");
    slot2Button.setButtonText("2");
    slot3Button.setButtonText("3");
    slot1OnAttachment = std::make_unique<ButtonAttachment>(state, "slot1On", slot1Button);
    slot2OnAttachment = std::make_unique<ButtonAttachment>(state, "slot2On", slot2Button);
    slot3OnAttachment = std::make_unique<ButtonAttachment>(state, "slot3On", slot3Button);

    addAndMakeVisible(modeLabel);
    modeLabel.setText("Mode", juce::dontSendNotification);
    modeLabel.attachToComponent(&modeBox, false);
    modeBox.setVisible(false);
    modeLabel.setVisible(false);

    addAndMakeVisible(qualityBox);
    qualityBox.addItem("Low 1024", 1);
    qualityBox.addItem("Normal 2048", 2);
    qualityBox.addItem("High 4096", 3);
    qualityBox.addItem("Ultra 8192", 4);
    qualityAttachment = std::make_unique<ComboBoxAttachment>(state, "fftQuality", qualityBox);

    addAndMakeVisible(qualityLabel);
    qualityLabel.setText("FFT", juce::dontSendNotification);
    qualityLabel.attachToComponent(&qualityBox, false);
    qualityLabel.setColour(juce::Label::textColourId, juce::Colour(cream));

   #if JucePlugin_Build_Standalone
    addAndMakeVisible(inputModeBox);
    inputModeBox.addItem("Stereo 1+2", 1);
    inputModeBox.addItem("Input 1 Mono", 2);
    inputModeBox.addItem("Input 2 Mono", 3);
    inputModeAttachment = std::make_unique<ComboBoxAttachment>(state, "inputMode", inputModeBox);

    addAndMakeVisible(inputModeLabel);
    inputModeLabel.setText("Input", juce::dontSendNotification);
    inputModeLabel.attachToComponent(&inputModeBox, false);
    inputModeLabel.setColour(juce::Label::textColourId, juce::Colour(cream));
   #endif

    addChildComponent(presetBox);
    presetBox.setTextWhenNothingSelected("Preset");
    presetBox.onChange = [this] { loadSelectedPreset(); };
    presetBox.setVisible(false);

    addAndMakeVisible(presetMenuButton);
    presetMenuButton.setButtonText("Preset");
    presetMenuButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff253434));
    presetMenuButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff324646));
    presetMenuButton.setColour(juce::TextButton::textColourOffId, juce::Colour(cream));
    presetMenuButton.onClick = [this] { showPresetMenu(); };

    addAndMakeVisible(savePresetButton);
    savePresetButton.setButtonText("SAVE");
    savePresetButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff241613));
    savePresetButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(coral));
    savePresetButton.setColour(juce::TextButton::textColourOffId, juce::Colour(cream));
    savePresetButton.onClick = [this] { savePreset(); };

    addAndMakeVisible(presetLabel);
    presetLabel.setText("PRESET", juce::dontSendNotification);
    presetLabel.attachToComponent(&presetMenuButton, false);
    presetLabel.setColour(juce::Label::textColourId, juce::Colour(mint));

    configureSlider(driveSlider, driveLabel, "Drive");
    configureSlider(rotSlider, rotLabel, "Rot");
    configureSlider(lockSlider, lockLabel, "Anchor");
    configureSlider(tiltSlider, tiltLabel, "Color");
    configureSlider(subGuardSlider, subGuardLabel, "Sub Lock");
    configureSlider(mixSlider, mixLabel, "Mix");
    configureSlider(outputSlider, outputLabel, "Output");
    configureSlider(widthSlider, widthLabel, "Width");
    configureSlider(inputGainSlider, inputGainLabel, "Input Gain");
    configureSlider(lowHoldSlider, lowHoldLabel, "High Pass");
    configureSlider(lowPassSlider, lowPassLabel, "Low Pass");
    configureSlider(reverseSlider, reverseLabel, "Sway");
    configureSlider(glassMixSlider, glassMixLabel, "Dry/Wet");
    configureSlider(harmonizerPitchSlider, harmonizerPitchLabel, "Pitch");
    configureSlider(harmonizerMixSlider, harmonizerMixLabel, "");
    configureSlider(harmonizer2PitchSlider, harmonizer2PitchLabel, "Pitch");
    configureSlider(harmonizer2MixSlider, harmonizer2MixLabel, "");
    inputGainSlider.setComponentID("knob-global");
    outputSlider.setComponentID("knob-global");
    lowHoldSlider.setComponentID("knob-global");
    lowPassSlider.setComponentID("knob-global");
    mixSlider.setComponentID("knob-global");
    widthSlider.setComponentID("knob-global");
    harmonizerPitchSlider.setComponentID("knob-harmonizer");
    harmonizerMixSlider.setComponentID("knob-harmonizer");
    harmonizer2PitchSlider.setComponentID("knob-harmonizer");
    harmonizer2MixSlider.setComponentID("knob-harmonizer");
    configureSlotSliderRanges();
    inputGainSlider.setNumDecimalPlacesToDisplay(1);
    outputSlider.setNumDecimalPlacesToDisplay(1);
    lowHoldSlider.setNumDecimalPlacesToDisplay(1);
    lowPassSlider.setNumDecimalPlacesToDisplay(1);
    mixSlider.setNumDecimalPlacesToDisplay(1);
    widthSlider.setNumDecimalPlacesToDisplay(1);
    harmonizerPitchSlider.setNumDecimalPlacesToDisplay(1);
    harmonizerMixSlider.setNumDecimalPlacesToDisplay(1);
    harmonizer2PitchSlider.setNumDecimalPlacesToDisplay(1);
    harmonizer2MixSlider.setNumDecimalPlacesToDisplay(1);
    mixSlider.textFromValueFunction = [] (double value) { return juce::String(value * 100.0, 1) + "%"; };
    mixSlider.valueFromTextFunction = [] (const juce::String& text) { return text.retainCharacters("0123456789.").getDoubleValue() / 100.0; };
    harmonizerMixSlider.textFromValueFunction = [] (double value) { return juce::String(value * 100.0, 1) + "%"; };
    harmonizerMixSlider.valueFromTextFunction = [] (const juce::String& text) { return text.retainCharacters("0123456789.").getDoubleValue() / 100.0; };
    harmonizer2MixSlider.textFromValueFunction = [] (double value) { return juce::String(value * 100.0, 1) + "%"; };
    harmonizer2MixSlider.valueFromTextFunction = [] (const juce::String& text) { return text.retainCharacters("0123456789.").getDoubleValue() / 100.0; };
    glassMixSlider.textFromValueFunction = [] (double value) { return juce::String(value * 100.0, 1) + "%"; };
    glassMixSlider.valueFromTextFunction = [] (const juce::String& text) { return text.retainCharacters("0123456789.").getDoubleValue() / 100.0; };
    mixSlider.setDoubleClickReturnValue(true, 1.0);
    outputSlider.setDoubleClickReturnValue(true, 0.0);
    widthSlider.setDoubleClickReturnValue(true, 0.0);
    inputGainSlider.setDoubleClickReturnValue(true, 0.0);
    lowHoldSlider.setDoubleClickReturnValue(true, 20.0);
    lowPassSlider.setDoubleClickReturnValue(true, 20000.0);
    harmonizerPitchSlider.setDoubleClickReturnValue(true, 0.0);
    harmonizerMixSlider.setDoubleClickReturnValue(true, 0.0);
    harmonizer2PitchSlider.setDoubleClickReturnValue(true, 0.0);
    harmonizer2MixSlider.setDoubleClickReturnValue(true, 0.0);

    addAndMakeVisible(edgePreButton);
    edgePreButton.setButtonText("Edge Pre");
    edgePreButton.setColour(juce::ToggleButton::textColourId, juce::Colour(0xfff7e8c8));
    edgePreButton.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffff5a36));

    addAndMakeVisible(harmonizerButton);
    harmonizerButton.setButtonText("Harmonizer");
    harmonizerButton.setColour(juce::ToggleButton::textColourId, juce::Colour(0xfff7e8c8));
    harmonizerButton.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffff5a36));

    addAndMakeVisible(harmonizer2Button);
    harmonizer2Button.setButtonText("Harmonizer 2");
    harmonizer2Button.setColour(juce::ToggleButton::textColourId, juce::Colour(0xfff7e8c8));
    harmonizer2Button.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffff5a36));

    addAndMakeVisible(freezeButton);
    freezeButton.setButtonText("Freeze");
    freezeButton.setColour(juce::ToggleButton::textColourId, juce::Colour(0xfff7e8c8));
    freezeButton.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffff5a36));

    mixAttachment = std::make_unique<SliderAttachment>(state, "mix", mixSlider);
    outputAttachment = std::make_unique<SliderAttachment>(state, "output", outputSlider);
    widthAttachment = std::make_unique<SliderAttachment>(state, "width", widthSlider);
    inputGainAttachment = std::make_unique<SliderAttachment>(state, "inputGain", inputGainSlider);
    lowHoldAttachment = std::make_unique<SliderAttachment>(state, "lowHold", lowHoldSlider);
    lowPassAttachment = std::make_unique<SliderAttachment>(state, "lowPass", lowPassSlider);
    harmonizerOnAttachment = std::make_unique<ButtonAttachment>(state, "harmonizerOn", harmonizerButton);
    harmonizer2OnAttachment = std::make_unique<ButtonAttachment>(state, "harmonizer2On", harmonizer2Button);
    freezeOnAttachment = std::make_unique<ButtonAttachment>(state, "freezeOn", freezeButton);
    harmonizerPitchAttachment = std::make_unique<SliderAttachment>(state, "harmonizerPitch", harmonizerPitchSlider);
    harmonizerMixAttachment = std::make_unique<SliderAttachment>(state, "harmonizerMix", harmonizerMixSlider);
    harmonizer2PitchAttachment = std::make_unique<SliderAttachment>(state, "harmonizer2Pitch", harmonizer2PitchSlider);
    harmonizer2MixAttachment = std::make_unique<SliderAttachment>(state, "harmonizer2Mix", harmonizer2MixSlider);

    connectSlotControlWriters();
    updateModeLabels();
    updateModeAttachments();
    refreshPresetMenu();
    startTimerHz(30);
    setSize(1480, 940);
}

SpectralRotAudioProcessorEditor::~SpectralRotAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void SpectralRotAudioProcessorEditor::timerCallback()
{
    processor.copyOutputWaveform(screenWaveform);
    repaint(scopeBoundsForModeIndex(selectedSlotModeIndex()).expanded(14.0f).toNearestInt());
}

juce::File SpectralRotAudioProcessorEditor::getPresetDirectory() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("trukendoos")
        .getChildFile("Presets");
}

void SpectralRotAudioProcessorEditor::refreshPresetMenu()
{
    loadingPresetMenu = true;
    presetBox.clear(juce::dontSendNotification);
    presetMenuFiles.clear();

    auto directory = getPresetDirectory();
    directory.createDirectory();

    juce::Array<juce::File> files;
    directory.findChildFiles(files, juce::File::findFiles, true, "*.trucje");
    files.sort();

    auto itemId = 1;
    for (const auto& file : files)
    {
        presetBox.addItem(file.getFileNameWithoutExtension(), itemId++);
        presetMenuFiles.add(file);
    }

    loadingPresetMenu = false;
}

void SpectralRotAudioProcessorEditor::showPresetMenu()
{
    saveSelectedSlotControls();
    refreshPresetMenu();

    auto directory = getPresetDirectory();
    directory.createDirectory();

    juce::PopupMenu menu;
    presetMenuFiles.clear();

    auto addPresetFile = [this, &menu] (const juce::File& file)
    {
        presetMenuFiles.add(file);
        menu.addItem(presetMenuFiles.size(), file.getFileNameWithoutExtension());
    };

    juce::Array<juce::File> rootFiles;
    directory.findChildFiles(rootFiles, juce::File::findFiles, false, "*.trucje");
    rootFiles.sort();

    for (const auto& file : rootFiles)
        addPresetFile(file);

    juce::Array<juce::File> folders;
    directory.findChildFiles(folders, juce::File::findDirectories, false);
    folders.sort();

    for (const auto& folder : folders)
    {
        juce::Array<juce::File> folderFiles;
        folder.findChildFiles(folderFiles, juce::File::findFiles, true, "*.trucje");
        folderFiles.sort();

        if (folderFiles.isEmpty())
            continue;

        juce::PopupMenu submenu;
        for (const auto& file : folderFiles)
        {
            presetMenuFiles.add(file);
            const auto relativePath = file.getRelativePathFrom(folder).replace("\\", " / ");
            submenu.addItem(presetMenuFiles.size(), relativePath.upToLastOccurrenceOf(".trucje", false, true));
        }

        menu.addSubMenu(folder.getFileName(), submenu);
    }

    if (presetMenuFiles.isEmpty())
        menu.addItem(1, "No .trucje presets found", false, false);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(presetMenuButton),
        [this] (int result)
        {
            if (result <= 0 || result > presetMenuFiles.size())
                return;

            loadPresetFile(presetMenuFiles.getReference(result - 1));
        });
}

void SpectralRotAudioProcessorEditor::savePreset()
{
    saveSelectedSlotControls();

    const auto directory = getPresetDirectory();
    directory.createDirectory();

    auto currentName = presetMenuButton.getButtonText();
    auto defaultName = currentName.isNotEmpty() && currentName != "Preset" ? currentName : juce::String("New Rot");
    auto defaultFile = directory.getChildFile(juce::File::createLegalFileName(defaultName)).withFileExtension(".trucje");
    presetChooser = std::make_unique<juce::FileChooser>("Save trukendoos preset", defaultFile, "*.trucje");

    presetChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& chooser)
        {
            auto file = chooser.getResult();

            if (file == juce::File())
                return;

            if (file.getFileExtension().isEmpty())
                file = file.withFileExtension(".trucje");

            if (auto xml = processor.getValueTreeState().copyState().createXml())
            {
                xml->setAttribute("presetFormat", "trukendoosState");
                xml->writeTo(file);
            }

            refreshPresetMenu();
            presetMenuButton.setButtonText(file.getFileNameWithoutExtension());
        });
}

void SpectralRotAudioProcessorEditor::loadPresetFile(const juce::File& file)
{
    if (! file.existsAsFile())
        return;

    auto xml = juce::parseXML(file);

    if (xml == nullptr)
        return;

    auto tree = juce::ValueTree::fromXml(*xml);

    if (! tree.isValid())
        return;

    processor.getValueTreeState().replaceState(tree);
    presetMenuButton.setButtonText(file.getFileNameWithoutExtension());
    updateModeLabels();
    updateModeAttachments();
    resized();
    repaint();
}

void SpectralRotAudioProcessorEditor::loadSelectedPreset()
{
    if (loadingPresetMenu || presetBox.getSelectedId() <= 0)
        return;

    const auto file = getPresetDirectory().getChildFile(presetBox.getText()).withFileExtension(".trucje");
    loadPresetFile(file);
}

void SpectralRotAudioProcessorEditor::selectSlot(int slotIndex)
{
    saveSelectedSlotControls();
    selectedSlot = juce::jlimit(0, 2, slotIndex);
    updateModeLabels();
    updateModeAttachments();
    resized();
    repaint();
}

int SpectralRotAudioProcessorEditor::slotAt(juce::Point<int> position) const
{
    for (int i = 0; i < static_cast<int>(slotBounds.size()); ++i)
        if (slotBounds[static_cast<size_t>(i)].contains(position))
            return i;

    return -1;
}

void SpectralRotAudioProcessorEditor::swapSlots(int first, int second)
{
    if (first == second || first < 0 || second < 0)
        return;

    auto& state = processor.getValueTreeState();
    const auto firstModeId = "slot" + juce::String(first + 1) + "Mode";
    const auto secondModeId = "slot" + juce::String(second + 1) + "Mode";
    const auto firstOnId = "slot" + juce::String(first + 1) + "On";
    const auto secondOnId = "slot" + juce::String(second + 1) + "On";

    auto* firstMode = parameterFor(state, firstModeId);
    auto* secondMode = parameterFor(state, secondModeId);
    auto* firstOn = parameterFor(state, firstOnId);
    auto* secondOn = parameterFor(state, secondOnId);

    const auto firstModeValue = firstMode->getValue();
    const auto firstOnValue = firstOn->getValue();
    float firstControls[8] {};
    float secondControls[8] {};
    const juce::String suffixes[] = { "Drive", "Rot", "Lock", "Tilt", "Shape", "Reverse", "GlassMix", "EdgePre" };

    for (int i = 0; i < 8; ++i)
    {
        firstControls[i] = parameterFor(state, "slot" + juce::String(first + 1) + suffixes[i])->getValue();
        secondControls[i] = parameterFor(state, "slot" + juce::String(second + 1) + suffixes[i])->getValue();
    }

    firstMode->setValueNotifyingHost(secondMode->getValue());
    firstOn->setValueNotifyingHost(secondOn->getValue());
    secondMode->setValueNotifyingHost(firstModeValue);
    secondOn->setValueNotifyingHost(firstOnValue);

    for (int i = 0; i < 8; ++i)
    {
        parameterFor(state, "slot" + juce::String(first + 1) + suffixes[i])->setValueNotifyingHost(secondControls[i]);
        parameterFor(state, "slot" + juce::String(second + 1) + suffixes[i])->setValueNotifyingHost(firstControls[i]);
    }

    selectedSlot = juce::jlimit(0, 2, second);
    updateModeLabels();
    updateModeAttachments();
    resized();
    repaint();
}

void SpectralRotAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    draggedSlot = slotAt(event.getPosition());
    draggingSlot = draggedSlot >= 0;
    dragPosition = event.getPosition();

    if (draggedSlot >= 0)
        selectSlot(draggedSlot);

    repaint();
}

void SpectralRotAudioProcessorEditor::mouseDrag(const juce::MouseEvent& event)
{
    if (draggedSlot >= 0)
    {
        draggingSlot = true;
        dragPosition = event.getPosition();
        repaint();
    }
}

void SpectralRotAudioProcessorEditor::mouseUp(const juce::MouseEvent& event)
{
    const auto targetSlot = slotAt(event.getPosition());

    if (draggedSlot >= 0 && targetSlot >= 0)
        swapSlots(draggedSlot, targetSlot);

    draggedSlot = -1;
    draggingSlot = false;
    repaint();
}

int SpectralRotAudioProcessorEditor::selectedSlotModeIndex() const
{
    const juce::ComboBox* boxes[] = { &slot1Box, &slot2Box, &slot3Box };
    return boxes[static_cast<size_t>(juce::jlimit(0, 2, selectedSlot))]->getSelectedItemIndex();
}

juce::String SpectralRotAudioProcessorEditor::selectedSlotPrefix() const
{
    return "slot" + juce::String(selectedSlot + 1);
}

void SpectralRotAudioProcessorEditor::updateModeAttachments()
{
    auto& state = processor.getValueTreeState();
    const auto prefix = selectedSlotPrefix();

    loadingSlotControls = true;
    driveSlider.setValue(getRealParameterValue(state, prefix + "Drive"), juce::dontSendNotification);
    rotSlider.setValue(getRealParameterValue(state, prefix + "Rot"), juce::dontSendNotification);
    lockSlider.setValue(getRealParameterValue(state, prefix + "Lock"), juce::dontSendNotification);
    tiltSlider.setValue(getRealParameterValue(state, prefix + "Tilt"), juce::dontSendNotification);
    subGuardSlider.setValue(getRealParameterValue(state, prefix + "Shape", 90.0f), juce::dontSendNotification);
    reverseSlider.setValue(getRealParameterValue(state, prefix + "Reverse"), juce::dontSendNotification);
    glassMixSlider.setValue(getRealParameterValue(state, prefix + "GlassMix", 1.0f), juce::dontSendNotification);
    edgePreButton.setToggleState(getRealParameterValue(state, prefix + "EdgePre") > 0.5f, juce::dontSendNotification);
    loadingSlotControls = false;
}

void SpectralRotAudioProcessorEditor::saveSelectedSlotControls()
{
    auto& state = processor.getValueTreeState();
    const auto prefix = selectedSlotPrefix();

    setRealParameterValue(state, prefix + "Drive", static_cast<float>(driveSlider.getValue()));
    setRealParameterValue(state, prefix + "Rot", static_cast<float>(rotSlider.getValue()));
    setRealParameterValue(state, prefix + "Lock", static_cast<float>(lockSlider.getValue()));
    setRealParameterValue(state, prefix + "Tilt", static_cast<float>(tiltSlider.getValue()));
    setRealParameterValue(state, prefix + "Shape", static_cast<float>(subGuardSlider.getValue()));
    setRealParameterValue(state, prefix + "Reverse", static_cast<float>(reverseSlider.getValue()));
    setRealParameterValue(state, prefix + "GlassMix", static_cast<float>(glassMixSlider.getValue()));
    setRealParameterValue(state, prefix + "EdgePre", edgePreButton.getToggleState() ? 1.0f : 0.0f);
}

void SpectralRotAudioProcessorEditor::connectSlotControlWriters()
{
    auto writeSelectedSlot = [this]
    {
        if (! loadingSlotControls)
            saveSelectedSlotControls();
    };

    driveSlider.onValueChange = writeSelectedSlot;
    rotSlider.onValueChange = writeSelectedSlot;
    lockSlider.onValueChange = writeSelectedSlot;
    tiltSlider.onValueChange = writeSelectedSlot;
    subGuardSlider.onValueChange = writeSelectedSlot;
    reverseSlider.onValueChange = writeSelectedSlot;
    glassMixSlider.onValueChange = writeSelectedSlot;
    edgePreButton.onClick = writeSelectedSlot;
}

void SpectralRotAudioProcessorEditor::updateModeLabels()
{
    const auto modeIndex = selectedSlotModeIndex();
    const auto isGlass = modeIndex == 2;
    const auto accent = modeAccentForIndex(modeIndex);
    reverseSlider.setVisible(isGlass);
    reverseLabel.setVisible(isGlass);
    glassMixSlider.setVisible(isGlass);
    glassMixLabel.setVisible(isGlass);
    subGuardSlider.setVisible(modeIndex != 0);
    subGuardLabel.setVisible(modeIndex != 0);
    edgePreButton.setVisible(modeIndex == 1);
    driveSlider.setRange(modeIndex == 0 ? -1.0 : 0.0, 1.0, 0.001);
    lockSlider.setRange(modeIndex == 2 ? -1.0 : 0.0, 1.0, 0.001);
    tiltSlider.setRange(0.0, 1.0, 0.001);

    juce::Slider* modeSliders[] = { &driveSlider, &rotSlider, &lockSlider, &tiltSlider, &subGuardSlider, &reverseSlider, &glassMixSlider };
    juce::Label* modeLabels[] = { &driveLabel, &rotLabel, &lockLabel, &tiltLabel, &subGuardLabel, &reverseLabel, &glassMixLabel };

    for (auto* slider : modeSliders)
    {
        slider->setComponentID(modeIndex == 1 ? "knob-rust" : (modeIndex == 2 ? "knob-glass" : "knob-melt"));
        slider->setColour(juce::Slider::rotarySliderFillColourId, accent);
        slider->setColour(juce::Slider::thumbColourId, accent.brighter(0.7f));
    }

    for (auto* label : modeLabels)
    {
        label->setColour(juce::Label::textColourId, juce::Colours::white);
        label->setFont(juce::FontOptions(13.5f, juce::Font::bold));
    }

    switch (modeIndex)
    {
        case 1:
            driveSlider.setDoubleClickReturnValue(true, 0.55);
            rotSlider.setDoubleClickReturnValue(true, 0.58);
            lockSlider.setDoubleClickReturnValue(true, 0.0);
            tiltSlider.setDoubleClickReturnValue(true, 0.0);
            subGuardSlider.setDoubleClickReturnValue(true, 20.0);
            reverseSlider.setDoubleClickReturnValue(true, 0.0);
            glassMixSlider.setDoubleClickReturnValue(true, 1.0);
            driveLabel.setText("Corrode", juce::dontSendNotification);
            rotLabel.setText("Fold", juce::dontSendNotification);
            lockLabel.setText("Redux", juce::dontSendNotification);
            tiltLabel.setText("Edge", juce::dontSendNotification);
            subGuardLabel.setText("Ring", juce::dontSendNotification);
            reverseLabel.setText("Sway", juce::dontSendNotification);
            break;

        case 2:
            driveSlider.setDoubleClickReturnValue(true, 0.35);
            rotSlider.setDoubleClickReturnValue(true, 0.35);
            lockSlider.setDoubleClickReturnValue(true, 0.0);
            tiltSlider.setDoubleClickReturnValue(true, 0.0);
            subGuardSlider.setDoubleClickReturnValue(true, 90.0);
            reverseSlider.setDoubleClickReturnValue(true, 0.0);
            glassMixSlider.setDoubleClickReturnValue(true, 1.0);
            driveLabel.setText("Time", juce::dontSendNotification);
            rotLabel.setText("Feedback", juce::dontSendNotification);
            lockLabel.setText("Texture", juce::dontSendNotification);
            tiltLabel.setText("Drift", juce::dontSendNotification);
            subGuardLabel.setText("Shatter", juce::dontSendNotification);
            reverseLabel.setText("Sway", juce::dontSendNotification);
            glassMixLabel.setText("Dry/Wet", juce::dontSendNotification);
            break;

        default:
            driveSlider.setDoubleClickReturnValue(true, 0.0);
            rotSlider.setDoubleClickReturnValue(true, 0.58);
            lockSlider.setDoubleClickReturnValue(true, 0.25);
            tiltSlider.setDoubleClickReturnValue(true, 0.62);
            subGuardSlider.setDoubleClickReturnValue(true, 90.0);
            reverseSlider.setDoubleClickReturnValue(true, 0.0);
            glassMixSlider.setDoubleClickReturnValue(true, 1.0);
            driveLabel.setText("Morph", juce::dontSendNotification);
            rotLabel.setText("Band Spread", juce::dontSendNotification);
            lockLabel.setText("Motion", juce::dontSendNotification);
            tiltLabel.setText("Depth", juce::dontSendNotification);
            subGuardLabel.setText("", juce::dontSendNotification);
            reverseLabel.setText("Sway", juce::dontSendNotification);
            break;
    }
}

void SpectralRotAudioProcessorEditor::configureSlider(juce::Slider& slider, juce::Label& label, const juce::String& text)
{
    addAndMakeVisible(slider);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 78, 22);
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffff5a36));
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff27221f));
    slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xfff7e8c8));

    addAndMakeVisible(label);
    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::FontOptions(13.5f, juce::Font::bold));
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::white);
}

void SpectralRotAudioProcessorEditor::configureSlotSliderRanges()
{
    driveSlider.setRange(0.0, 1.0, 0.001);
    rotSlider.setRange(0.0, 1.0, 0.001);
    lockSlider.setRange(0.0, 1.0, 0.001);
    tiltSlider.setRange(-1.0, 1.0, 0.001);
    subGuardSlider.setRange(20.0, 260.0, 0.1);
    reverseSlider.setRange(0.0, 1.0, 0.001);
    glassMixSlider.setRange(0.0, 1.0, 0.001);

    juce::Slider* slotSliders[] = { &driveSlider, &rotSlider, &lockSlider, &tiltSlider, &subGuardSlider, &reverseSlider, &glassMixSlider };

    for (auto* slider : slotSliders)
    {
        slider->setNumDecimalPlacesToDisplay(1);
        slider->setDoubleClickReturnValue(true, slider->getMinimum());
    }
}

void SpectralRotAudioProcessorEditor::drawScrew(juce::Graphics& g, juce::Point<float> centre, float radius, float angle) const
{
    auto bounds = juce::Rectangle<float>(radius * 2.0f, radius * 2.0f).withCentre(centre);
    juce::ColourGradient screw(juce::Colour(0xfff0dfb6), bounds.getX(), bounds.getY(),
                               juce::Colour(0xff5f5247), bounds.getRight(), bounds.getBottom(), false);
    g.setGradientFill(screw);
    g.fillEllipse(bounds);
    g.setColour(juce::Colour(ink));
    g.drawEllipse(bounds, 1.0f);

    juce::Path slot;
    slot.addRoundedRectangle(-radius * 0.72f, -1.1f, radius * 1.44f, 2.2f, 1.1f);
    slot.applyTransform(juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
    g.setColour(juce::Colour(0xcc17130f));
    g.fillPath(slot);
}

void SpectralRotAudioProcessorEditor::drawScreen(juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    g.setColour(juce::Colour(0xff080706));
    g.fillRoundedRectangle(bounds.expanded(8.0f), 8.0f);
    g.setColour(juce::Colour(0xff443a32));
    g.drawRoundedRectangle(bounds.expanded(8.0f), 8.0f, 2.0f);

    juce::ColourGradient glass(juce::Colour(0xff09302f), bounds.getX(), bounds.getY(),
                               juce::Colour(0xff07100f), bounds.getRight(), bounds.getBottom(), false);
    g.setGradientFill(glass);
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(juce::Colour(0x221bafa8));
    for (int y = 0; y < bounds.getHeight(); y += 5)
        g.drawHorizontalLine(static_cast<int>(bounds.getY()) + y, bounds.getX(), bounds.getRight());

    const auto centreY = bounds.getCentreY();
    juce::Path spectrum;
    spectrum.startNewSubPath(bounds.getX() + 10.0f, centreY);
    const auto displayWidth = bounds.getWidth() - 20.0f;
    for (int i = 0; i < static_cast<int>(screenWaveform.size()); ++i)
    {
        const auto x = bounds.getX() + 10.0f + displayWidth * static_cast<float>(i) / static_cast<float>(screenWaveform.size() - 1);
        const auto y = centreY - juce::jlimit(-1.0f, 1.0f, screenWaveform[static_cast<size_t>(i)]) * (bounds.getHeight() * 0.38f);
        spectrum.lineTo(x, y);
    }

    g.setColour(juce::Colour(mint).withAlpha(0.22f));
    g.strokePath(spectrum, juce::PathStrokeType(10.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(juce::Colour(mint));
    g.strokePath(spectrum, juce::PathStrokeType(2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour(juce::Colour(cream).withAlpha(0.7f));
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText("OUTPUT SCOPE", bounds.reduced(14.0f).toNearestInt(), juce::Justification::topLeft);
    g.drawText("SLOT " + juce::String(selectedSlot + 1), bounds.reduced(14.0f).toNearestInt(), juce::Justification::bottomRight);
}

void SpectralRotAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff080605));
    drawImageToFill(g, backplateImage(), getLocalBounds().toFloat());
    drawImageToFill(g, presetPlateImage(), { 533.0f, 29.0f, 414.0f, 164.0f });
    drawImageWithin(g, inputBadgeImage(), { 34.0f, 34.0f, 140.0f, 140.0f });
    drawImageWithin(g, outputBadgeImage(), { 1301.0f, 34.0f, 140.0f, 140.0f });
    drawImageToFill(g, globalBusImage(), { 192.0f, 680.0f, 1096.0f, 290.0f });

    auto& state = processor.getValueTreeState();

    for (int i = 0; i < 3; ++i)
    {
        const auto slot = slotBounds[static_cast<size_t>(i)].toFloat();
        const auto modeIndex = juce::jlimit(0, 2, static_cast<int>(state.getRawParameterValue("slot" + juce::String(i + 1) + "Mode")->load()));
        const auto enabled = state.getRawParameterValue("slot" + juce::String(i + 1) + "On")->load() > 0.5f;
        const auto selected = i == selectedSlot;
        const auto held = draggingSlot && draggedSlot == i;
        auto drawSlot = slot.toFloat();

        if (held)
        {
            drawImageWithin(g, headerImageForMode(modeIndex), slot, 0.55f);
            drawSlot = slot.expanded(10.0f, 7.0f);
            drawSlot.setCentre(dragPosition.toFloat());
            g.setColour(juce::Colour(0x6617130f));
            g.fillRoundedRectangle(drawSlot.expanded(7.0f), 12.0f);
        }

        juce::ignoreUnused(enabled);
        g.setColour(modeFillForIndex(modeIndex).darker(0.2f));
        g.fillRoundedRectangle(drawSlot.reduced(12.0f, 16.0f), 10.0f);
        drawImageWithin(g, headerImageForMode(modeIndex), drawSlot, 1.0f);
        juce::ignoreUnused(selected);
    }

    const auto selectedMode = selectedSlotModeIndex();
    drawImageWithin(g, pedalImageForMode(selectedMode), modePedalBoundsForIndex(selectedMode));
    drawImageWithin(g, harmonizerPedalImage(), { 1083.0f, 170.0f, 348.0f, 533.0f });
    drawScreen(g, scopeBoundsForModeIndex(selectedMode));
}

void SpectralRotAudioProcessorEditor::resized()
{
    constexpr int knobSize = 94;
    constexpr int smallKnobSize = 76;
    constexpr int smallKnobTotalHeight = 100;

    auto placeKnob = [=] (juce::Label& label, juce::Slider& slider, int centreX, int topY)
    {
        label.setBounds(centreX - 70, topY, 140, 22);
        slider.setBounds(centreX - knobSize / 2, topY + 23, knobSize, knobSize);
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    };

    auto placeSmallKnob = [=] (juce::Label& label, juce::Slider& slider, int centreX, int topY)
    {
        label.setBounds(centreX - 60, topY, 120, 20);
        slider.setBounds(centreX - smallKnobSize / 2, topY + 22, smallKnobSize, smallKnobTotalHeight);
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 78, 20);
    };

    auto layoutSlot = [] (juce::Rectangle<int> bounds, juce::ToggleButton& button, juce::ComboBox& box)
    {
        button.setBounds(bounds.removeFromLeft(34).withSizeKeepingCentre(22, 36));
        box.setBounds(bounds.reduced(6, 24));
    };

    slotBounds[0] = juce::Rectangle<int>(102, 137, 286, 150);
    slotBounds[1] = juce::Rectangle<int>(102, 330, 286, 150);
    slotBounds[2] = juce::Rectangle<int>(102, 526, 286, 150);
    layoutSlot(slotBounds[0], slot1Button, slot1Box);
    layoutSlot(slotBounds[1], slot2Button, slot2Box);
    layoutSlot(slotBounds[2], slot3Button, slot3Box);

    placeSmallKnob(inputGainLabel, inputGainSlider, 103, 47);
    placeSmallKnob(outputLabel, outputSlider, 1371, 46);

    const auto isGlass = selectedSlotModeIndex() == 2;
    const auto isMelt = selectedSlotModeIndex() == 0;
    const auto isRust = selectedSlotModeIndex() == 1;

    juce::Slider* sliders[] = {
        &driveSlider, &rotSlider, &lockSlider, &tiltSlider, &subGuardSlider, &reverseSlider, &glassMixSlider
    };

    juce::Label* labels[] = {
        &driveLabel, &rotLabel, &lockLabel, &tiltLabel, &subGuardLabel, &reverseLabel, &glassMixLabel
    };

    juce::Point<int> modePositions[7] {};

    if (isGlass)
    {
        modePositions[0] = { 598, 271 };
        modePositions[1] = { 685, 271 };
        modePositions[2] = { 782, 271 };
        modePositions[3] = { 636, 398 };
        modePositions[4] = { 735, 398 };
        modePositions[5] = { 872, 271 };
        modePositions[6] = { 835, 398 };
    }
    else if (isRust)
    {
        modePositions[0] = { 648, 228 };
        modePositions[1] = { 738, 311 };
        modePositions[2] = { 827, 227 };
        modePositions[3] = { 649, 382 };
        modePositions[4] = { 827, 382 };
        modePositions[5] = { 827, 227 };
        modePositions[6] = { 835, 398 };
    }
    else
    {
        modePositions[0] = { 639, 230 };
        modePositions[1] = { 830, 230 };
        modePositions[2] = { 639, 377 };
        modePositions[3] = { 830, 377 };
        modePositions[4] = { 830, 377 };
        modePositions[5] = { 830, 377 };
        modePositions[6] = { 835, 398 };
    }

    for (int i = 0; i < 7; ++i)
    {
        const auto visible = i < (isGlass ? 7 : (isMelt ? 4 : 5));
        labels[i]->setVisible(visible);
        sliders[i]->setVisible(visible);

        if (! visible)
            continue;

        placeKnob(*labels[i], *sliders[i], modePositions[i].x, modePositions[i].y);
    }

    qualityBox.setBounds(415, 155, 115, 28);
   #if JucePlugin_Build_Standalone
    inputModeBox.setBounds(955, 155, 130, 28);
   #endif
    presetBox.setBounds(623, 79, 190, 64);
    presetMenuButton.setBounds(623, 79, 190, 64);
    savePresetButton.setBounds(880, 96, 72, 28);
    edgePreButton.setBounds(599, 518, 93, 28);
    edgePreButton.setVisible(isRust);
    harmonizerButton.setBounds(1133, 212, 110, 26);
    freezeButton.setBounds(1276, 902, 120, 26);

    placeSmallKnob(harmonizerPitchLabel, harmonizerPitchSlider, 1194, 256);
    placeSmallKnob(harmonizerMixLabel, harmonizerMixSlider, 1194, 358);

    harmonizer2Button.setBounds(1271, 212, 110, 26);
    placeSmallKnob(harmonizer2PitchLabel, harmonizer2PitchSlider, 1320, 256);
    placeSmallKnob(harmonizer2MixLabel, harmonizer2MixSlider, 1320, 358);

    juce::Slider* globalSliders[] = { &lowHoldSlider, &lowPassSlider, &mixSlider, &widthSlider };
    juce::Label* globalLabels[] = { &lowHoldLabel, &lowPassLabel, &mixLabel, &widthLabel };
    const juce::Point<int> globalPositions[] = {
        { 438, 765 },
        { 324, 765 },
        { 719, 765 },
        { 592, 765 }
    };

    for (int i = 0; i < 4; ++i)
    {
        placeSmallKnob(*globalLabels[i], *globalSliders[i], globalPositions[i].x, globalPositions[i].y);
    }
}
