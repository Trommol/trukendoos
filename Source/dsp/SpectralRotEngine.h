#pragma once

#include <juce_dsp/juce_dsp.h>

class SpectralRotEngine
{
public:
    enum class Mode
    {
        melt = 0,
        rust,
        glass
    };

    struct Parameters
    {
        Mode mode = Mode::melt;
        float drive = 0.0f;
        float rot = 0.0f;
        float lock = 0.5f;
        float tilt = 0.0f;
        float subGuardHz = 90.0f;
        float mix = 1.0f;
        float outputGain = 1.0f;
        float inputGain = 1.0f;
        float lowHoldHz = 90.0f;
        float width = 0.0f;
        float glassReverse = 0.0f;
        float glassMix = 1.0f;
        bool applyInputGain = true;
        bool applyOutputGain = true;
        bool applyWidth = true;
        bool applyHarmonizer = true;
        bool freezeEnabled = false;
        bool harmonizerEnabled = false;
        float harmonizerPitchSemitones = 0.0f;
        float harmonizerMix = 0.0f;
        bool harmonizer2Enabled = false;
        float harmonizer2PitchSemitones = 0.0f;
        float harmonizer2Mix = 0.0f;
        bool edgeBeforeCorrode = false;
        float lowPassHz = 20000.0f;
        int fftOrder = 11;
    };

    void prepare(double newSampleRate, int maximumBlockSize, int channels);
    void reset();
    void setParameters(const Parameters& newParameters);
    int getLatencySamples() const noexcept;
    void process(juce::AudioBuffer<float>& buffer);
    void processGlobalBus(juce::AudioBuffer<float>& buffer);

private:
    static constexpr int overlap = 4;
    static constexpr int maxSpectralDelayFrames = 128;

    struct ChannelState
    {
        std::vector<float> inputRing;
        std::vector<float> outputRing;
        std::vector<float> fftData;
        std::vector<float> originalMagnitudes;
        std::vector<float> processedMagnitudes;
        std::vector<float> spectralAdditions;
        std::vector<float> glassFeedbackReal;
        std::vector<float> glassFeedbackImag;
        std::vector<float> glassDelay;
        std::vector<float> spectralHistoryReal;
        std::vector<float> spectralHistoryImag;
        std::vector<float> harmonizerDelay;
        std::vector<float> harmonizer2Delay;
        int writePosition = 0;
        int harmonizerWritePosition = 0;
        int harmonizer2WritePosition = 0;
        int glassDelayWritePosition = 0;
        int spectralHistoryWritePosition = 0;
        int hopCounter = 0;
        int frameCounter = 0;
        float stereoSign = 1.0f;
        float harmonizerPhaseA = 0.0f;
        float harmonizerPhaseB = 0.5f;
        float harmonizer2PhaseA = 0.0f;
        float harmonizer2PhaseB = 0.5f;
        float phase = 0.0f;
        float slowPhase = 0.0f;
        float rmsCompensation = 1.0f;
        float corrodeEnvelope = 0.0f;
        float pink0 = 0.0f;
        float pink1 = 0.0f;
        float pink2 = 0.0f;
        float lowPassY1 = 0.0f;
        float rustCrushHeldSample = 0.0f;
        float highPassX1 = 0.0f;
        float highPassY1 = 0.0f;
        std::array<float, 8> phaserX {};
        std::array<float, 8> phaserY {};
        std::array<float, 4> vowelBand {};
        std::array<float, 4> vowelLow {};
        float phaserFeedback = 0.0f;
        float ringPhase = 0.0f;
        float ringEnvelope = 0.0f;
        float ringLowpass = 0.0f;
        float lastRingInput = 0.0f;
        float ringPeriodSamples = 96.0f;
        int ringZeroCrossCounter = 0;
        int rustCrushCounter = 0;
        unsigned int noiseState = 0x12345678u;
    };

    void processSample(ChannelState& state, float input, float& output);
    float processGlassDelaySample(ChannelState& state, float input) const;
    float processHarmonizer(ChannelState& state, float input, bool second);
    float processGonkRing(ChannelState& state, float input, float amount);
    float processHighPass(ChannelState& state, float input) const;
    float processLowPass(ChannelState& state, float input) const;
    float processRustPhaser(ChannelState& state, float input);
    float processRustCorrodeSample(ChannelState& state, float input);
    float readHarmonizerDelay(const ChannelState& state, float delaySamples, bool second) const;
    float processRustBitcrush(ChannelState& state, float input) const;
    void processFrame(ChannelState& state);
    bool hasGlassTail(const ChannelState& state) const;
    int spectralHistoryIndex(int frameOffset, int bin, int writePosition) const;
    int glassDelayIndex(int frameOffset, int bin, int writePosition) const;
    float calculateLocalAverage(const std::vector<float>& magnitudes, int bin) const;
    float protectionForFrequency(float frequencyHz) const;
    float modeLockAmount() const;
    float modeTargetRmsMultiplier() const;
    float modeOutputTrim() const;
    float nextNoise(ChannelState& state);
    void configureFft(int newFftOrder, int channels);
    void allocateChannelState(ChannelState& state);
    void prepareWidthDelay();
    float readWidthDelay(float delaySamples) const;
    void applyWidth(juce::AudioBuffer<float>& buffer, int channels, int samples);

    std::unique_ptr<juce::dsp::FFT> fft;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;
    std::vector<ChannelState> channelStates;
    Parameters parameters;
    double sampleRate = 44100.0;
    int currentFftOrder = 11;
    int fftSize = 1 << 11;
    int hopSize = (1 << 11) / overlap;
    int numBins = ((1 << 11) / 2) + 1;
    int currentChannels = 2;
    std::vector<float> widthDelay;
    int widthWritePosition = 0;
    float widthPhase = 0.0f;
    float olaCompensation = 2.0f / 3.0f;
};
