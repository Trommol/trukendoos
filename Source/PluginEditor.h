#pragma once

#include <array>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class SpectralRotAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                             private juce::Timer
{
public:
    explicit SpectralRotAudioProcessorEditor(SpectralRotAudioProcessor&);
    ~SpectralRotAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void timerCallback() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    class RotLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        RotLookAndFeel();

        void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                              float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                              juce::Slider& slider) override;
        void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
        void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                          int buttonX, int buttonY, int buttonW, int buttonH,
                          juce::ComboBox& box) override;
        juce::Font getComboBoxFont(juce::ComboBox&) override;
    };

    void configureSlider(juce::Slider& slider, juce::Label& label, const juce::String& text);
    void configureSlotSliderRanges();
    void updateModeLabels();
    void updateModeAttachments();
    void saveSelectedSlotControls();
    void connectSlotControlWriters();
    juce::String selectedSlotPrefix() const;
    int selectedSlotModeIndex() const;
    void selectSlot(int slotIndex);
    void swapSlots(int first, int second);
    int slotAt(juce::Point<int> position) const;
    void drawScrew(juce::Graphics& g, juce::Point<float> centre, float radius, float angle) const;
    void drawScreen(juce::Graphics& g, juce::Rectangle<float> bounds) const;
    juce::File getPresetDirectory() const;
    void refreshPresetMenu();
    void showPresetMenu();
    void savePreset();
    void loadPresetFile(const juce::File& file);
    void loadSelectedPreset();

    SpectralRotAudioProcessor& processor;
    RotLookAndFeel rotLookAndFeel;

    juce::ComboBox modeBox;
    juce::ComboBox slot1Box;
    juce::ComboBox slot2Box;
    juce::ComboBox slot3Box;
    juce::ComboBox qualityBox;
    juce::ComboBox inputModeBox;
    juce::ComboBox presetBox;
    juce::TextButton presetMenuButton;
    juce::TextButton savePresetButton;
    juce::ToggleButton slot1Button;
    juce::ToggleButton slot2Button;
    juce::ToggleButton slot3Button;
    juce::Label modeLabel;
    juce::Label qualityLabel;
    juce::Label inputModeLabel;
    juce::Label presetLabel;
    std::unique_ptr<ComboBoxAttachment> modeAttachment;
    std::unique_ptr<ComboBoxAttachment> slot1Attachment;
    std::unique_ptr<ComboBoxAttachment> slot2Attachment;
    std::unique_ptr<ComboBoxAttachment> slot3Attachment;
    std::unique_ptr<ComboBoxAttachment> qualityAttachment;
    std::unique_ptr<ComboBoxAttachment> inputModeAttachment;
    std::unique_ptr<ButtonAttachment> slot1OnAttachment;
    std::unique_ptr<ButtonAttachment> slot2OnAttachment;
    std::unique_ptr<ButtonAttachment> slot3OnAttachment;

    juce::Slider driveSlider;
    juce::Slider rotSlider;
    juce::Slider lockSlider;
    juce::Slider tiltSlider;
    juce::Slider subGuardSlider;
    juce::Slider mixSlider;
    juce::Slider outputSlider;
    juce::Slider widthSlider;
    juce::Slider inputGainSlider;
    juce::Slider lowHoldSlider;
    juce::Slider lowPassSlider;
    juce::Slider reverseSlider;
    juce::Slider glassMixSlider;
    juce::ToggleButton edgePreButton;
    juce::ToggleButton harmonizerButton;
    juce::ToggleButton harmonizer2Button;
    juce::ToggleButton freezeButton;
    juce::Slider harmonizerPitchSlider;
    juce::Slider harmonizerMixSlider;
    juce::Slider harmonizer2PitchSlider;
    juce::Slider harmonizer2MixSlider;

    juce::Label driveLabel;
    juce::Label rotLabel;
    juce::Label lockLabel;
    juce::Label tiltLabel;
    juce::Label subGuardLabel;
    juce::Label mixLabel;
    juce::Label outputLabel;
    juce::Label widthLabel;
    juce::Label inputGainLabel;
    juce::Label lowHoldLabel;
    juce::Label lowPassLabel;
    juce::Label reverseLabel;
    juce::Label glassMixLabel;
    juce::Label harmonizerPitchLabel;
    juce::Label harmonizerMixLabel;
    juce::Label harmonizer2PitchLabel;
    juce::Label harmonizer2MixLabel;

    std::unique_ptr<SliderAttachment> driveAttachment;
    std::unique_ptr<SliderAttachment> rotAttachment;
    std::unique_ptr<SliderAttachment> lockAttachment;
    std::unique_ptr<SliderAttachment> tiltAttachment;
    std::unique_ptr<SliderAttachment> subGuardAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<SliderAttachment> outputAttachment;
    std::unique_ptr<SliderAttachment> widthAttachment;
    std::unique_ptr<SliderAttachment> inputGainAttachment;
    std::unique_ptr<SliderAttachment> lowHoldAttachment;
    std::unique_ptr<SliderAttachment> lowPassAttachment;
    std::unique_ptr<SliderAttachment> reverseAttachment;
    std::unique_ptr<ButtonAttachment> harmonizerOnAttachment;
    std::unique_ptr<ButtonAttachment> harmonizer2OnAttachment;
    std::unique_ptr<ButtonAttachment> freezeOnAttachment;
    std::unique_ptr<SliderAttachment> harmonizerPitchAttachment;
    std::unique_ptr<SliderAttachment> harmonizerMixAttachment;
    std::unique_ptr<SliderAttachment> harmonizer2PitchAttachment;
    std::unique_ptr<SliderAttachment> harmonizer2MixAttachment;
    std::array<juce::Rectangle<int>, 3> slotBounds;
    int draggedSlot = -1;
    int selectedSlot = 0;
    bool draggingSlot = false;
    bool loadingSlotControls = false;
    bool loadingPresetMenu = false;
    juce::Point<int> dragPosition;
    std::array<float, 256> screenWaveform {};
    juce::Array<juce::File> presetMenuFiles;
    std::unique_ptr<juce::FileChooser> presetChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralRotAudioProcessorEditor)
};
