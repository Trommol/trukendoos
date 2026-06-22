#pragma once

#include <array>
#include <juce_audio_basics/juce_audio_basics.h>

class SpectralCompressor
{
public:
    struct Parameters
    {
        bool enabled = false;
        float tame = 0.0f;
        float thresholdDb = -18.0f;
        float focus = 0.5f;
        float speed = 0.45f;
        int fftOrder = 11;
    };

    void prepare(double newSampleRate, int channels);
    void reset();
    void setParameters(const Parameters& newParameters);
    int getLatencySamples() const noexcept { return 0; }
    void process(juce::AudioBuffer<float>& buffer);
    float getLastGainReduction() const noexcept { return lastGainReduction; }

private:
    static constexpr int bandCount = 10;

    struct ChannelState
    {
        std::array<float, bandCount - 1> lowpass {};
        std::array<float, bandCount> envelope {};
        std::array<float, bandCount> average {};
        std::array<float, bandCount> gain {};
        float activityLevel = 1.0e-4f;
        int transientProtectSamples = 0;
    };

    std::array<float, bandCount - 1> coefficients {};
    std::array<float, bandCount> focusWeights {};
    std::vector<ChannelState> states;
    Parameters parameters;
    double sampleRate = 44100.0;
    float lastGainReduction = 0.0f;

    void updateCoefficients();
};
