#pragma once

#include <array>
#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/SpectralCompressor.h"
#include "dsp/SpectralRotEngine.h"

class SpectralRotAudioProcessor final : public juce::AudioProcessor
{
public:
    enum class ChainMode
    {
        melt = 0,
        rust,
        glass,
        squash
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
    void copyCompressorWaveform(int slot, std::array<float, 256>& destination) const;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    void updateEngineParameters();
    void updateCompressorParameters();
    SpectralCompressor::Parameters makeCompressorParametersForSlot(int slot);
    SpectralRotEngine::Parameters makeParametersForSlot(int slot, ChainMode mode, bool firstActive, bool lastActive, bool chainHasActiveSlot);
    int getFftOrder() const;
    int calculateReportedLatencySamples();
    void applyInputMode(juce::AudioBuffer<float>& buffer);
    void delayDryBuffer(juce::AudioBuffer<float>& buffer, int delaySamples);
    void pushOutputWaveform(const juce::AudioBuffer<float>& buffer);
    void pushCompressorActivity(int slot, float activity);

    juce::AudioProcessorValueTreeState parameters;
    std::array<SpectralRotEngine, 4> engines;
    std::array<SpectralCompressor, 4> compressors;
    std::vector<std::vector<float>> dryDelayLines;
    std::vector<int> dryDelayPositions;
    std::array<std::atomic<float>, 1024> outputWaveform {};
    std::atomic<int> outputWaveformWritePosition { 0 };
    std::array<std::array<std::atomic<float>, 1024>, 4> compressorWaveforms {};
    std::array<std::atomic<int>, 4> compressorWaveformWritePositions {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralRotAudioProcessor)
};
