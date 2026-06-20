#pragma once

#include <array>
#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/SpectralRotEngine.h"

class SpectralRotAudioProcessor final : public juce::AudioProcessor
{
public:
    enum class ChainMode
    {
        melt = 0,
        rust,
        glass
    };

    SpectralRotAudioProcessor();
    ~SpectralRotAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept { return parameters; }
    void copyOutputWaveform(std::array<float, 256>& destination) const;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    void updateEngineParameters();
    SpectralRotEngine::Parameters makeParametersForSlot(int slot, ChainMode mode, bool firstActive, bool lastActive);
    int getFftOrder() const;
    int calculateReportedLatencySamples();
    void applyInputMode(juce::AudioBuffer<float>& buffer);
    void pushOutputWaveform(const juce::AudioBuffer<float>& buffer);

    juce::AudioProcessorValueTreeState parameters;
    std::array<SpectralRotEngine, 3> engines;
    std::array<std::atomic<float>, 1024> outputWaveform {};
    std::atomic<int> outputWaveformWritePosition { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralRotAudioProcessor)
};
