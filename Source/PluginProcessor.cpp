#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr const char* modeId = "mode";
    constexpr const char* mixId = "mix";
    constexpr const char* outputId = "output";
    constexpr const char* widthId = "width";
    constexpr const char* inputGainId = "inputGain";
    constexpr const char* lowHoldId = "lowHold";
    constexpr const char* glassReverseId = "glassReverse";
    constexpr const char* inputModeId = "inputMode";
    constexpr const char* harmonizerOnId = "harmonizerOn";
    constexpr const char* harmonizerPitchId = "harmonizerPitch";
    constexpr const char* harmonizerMixId = "harmonizerMix";
    constexpr const char* harmonizer2OnId = "harmonizer2On";
    constexpr const char* harmonizer2PitchId = "harmonizer2Pitch";
    constexpr const char* harmonizer2MixId = "harmonizer2Mix";
    constexpr const char* lowPassId = "lowPass";
    constexpr const char* fftQualityId = "fftQuality";
    constexpr const char* compressorOnId = "compressorOn";
    constexpr const char* compressorTameId = "compressorTame";
    constexpr const char* compressorThresholdId = "compressorThreshold";
    constexpr const char* compressorFocusId = "compressorFocus";
    constexpr const char* compressorSpeedId = "compressorSpeed";

    juce::String modePrefix(SpectralRotAudioProcessor::ChainMode mode)
    {
        switch (mode)
        {
            case SpectralRotAudioProcessor::ChainMode::rust: return "rust";
            case SpectralRotAudioProcessor::ChainMode::glass: return "glass";
            case SpectralRotAudioProcessor::ChainMode::squash: return "squash";
            default: return "melt";
        }
    }

    juce::String modeParam(SpectralRotAudioProcessor::ChainMode mode, const juce::String& suffix)
    {
        return modePrefix(mode) + suffix;
    }

    SpectralRotAudioProcessor::ChainMode chainModeFromIndex(int index)
    {
        switch (index)
        {
            case 1: return SpectralRotAudioProcessor::ChainMode::rust;
            case 2: return SpectralRotAudioProcessor::ChainMode::glass;
            case 3: return SpectralRotAudioProcessor::ChainMode::squash;
            default: return SpectralRotAudioProcessor::ChainMode::melt;
        }
    }

    SpectralRotEngine::Mode engineModeFromChain(SpectralRotAudioProcessor::ChainMode mode)
    {
        switch (mode)
        {
            case SpectralRotAudioProcessor::ChainMode::rust: return SpectralRotEngine::Mode::rust;
            case SpectralRotAudioProcessor::ChainMode::glass: return SpectralRotEngine::Mode::glass;
            case SpectralRotAudioProcessor::ChainMode::squash: break;
            default: return SpectralRotEngine::Mode::melt;
        }

        return SpectralRotEngine::Mode::melt;
    }

    float getParameterValue(juce::AudioProcessorValueTreeState& state, const juce::String& id)
    {
        return state.getRawParameterValue(id)->load();
    }

    void removeParameterFromState(juce::ValueTree tree, const juce::String& parameterId)
    {
        for (int child = tree.getNumChildren(); --child >= 0;)
        {
            auto childTree = tree.getChild(child);
            removeParameterFromState(childTree, parameterId);

            if (childTree.getProperty("id").toString() == parameterId)
                tree.removeChild(child, nullptr);
        }

        tree.removeProperty(parameterId, nullptr);
    }

    void setParameterValue(juce::AudioProcessorValueTreeState& state, const juce::String& id, float value)
    {
        if (auto* parameter = state.getParameter(id))
            parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    }

    juce::AudioParameterFloatAttributes percentAttributes()
    {
        return juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction([] (float value, int) { return juce::String(value * 100.0f, 1) + "%"; })
            .withValueFromStringFunction([] (const juce::String& text) { return text.retainCharacters("0123456789.").getFloatValue() / 100.0f; });
    }

    void addModeParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                           SpectralRotAudioProcessor::ChainMode mode,
                           float driveDefault,
                           float rotDefault,
                           float lockDefault,
                           float tiltDefault,
                           float shapeDefault)
    {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { modeParam(mode, "Drive"), 1 }, modePrefix(mode) + " Drive",
            juce::NormalisableRange<float> { -1.0f, 1.0f, 0.001f }, driveDefault));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { modeParam(mode, "Rot"), 1 }, modePrefix(mode) + " Rot",
            juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, rotDefault));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { modeParam(mode, "Lock"), 1 }, modePrefix(mode) + " Lock",
            juce::NormalisableRange<float> { -1.0f, 1.0f, 0.001f }, lockDefault));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { modeParam(mode, "Tilt"), 1 }, modePrefix(mode) + " Tilt",
            juce::NormalisableRange<float> { -1.0f, 1.0f, 0.001f }, tiltDefault));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { modeParam(mode, "Shape"), 1 }, modePrefix(mode) + " Shape",
            juce::NormalisableRange<float> { 20.0f, 260.0f, 0.1f }, shapeDefault));
    }

    void addSlotParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                           int slot,
                           float driveDefault,
                           float rotDefault,
                           float lockDefault,
                           float tiltDefault,
                           float shapeDefault)
    {
        const auto prefix = "slot" + juce::String(slot);

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "Drive", 1 }, prefix + " Drive",
            juce::NormalisableRange<float> { -1.0f, 1.0f, 0.001f }, driveDefault));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "Rot", 1 }, prefix + " Rot",
            juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, rotDefault));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "Lock", 1 }, prefix + " Lock",
            juce::NormalisableRange<float> { -1.0f, 1.0f, 0.001f }, lockDefault));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "Tilt", 1 }, prefix + " Tilt",
            juce::NormalisableRange<float> { -1.0f, 1.0f, 0.001f }, tiltDefault));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "Shape", 1 }, prefix + " Shape",
            juce::NormalisableRange<float> { 20.0f, 260.0f, 0.1f }, shapeDefault));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "Reverse", 1 }, prefix + " Reverse",
            juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "GlassMix", 1 }, prefix + " Glass Mix",
            juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 1.0f));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { prefix + "EdgePre", 1 }, prefix + " Edge Pre", false));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "SquashTame", 1 }, prefix + " Squash Tame",
            juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "SquashThreshold", 1 }, prefix + " Squash Threshold",
            juce::NormalisableRange<float> { -48.0f, 0.0f, 0.01f }, -18.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "SquashFocus", 1 }, prefix + " Squash Focus",
            juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { prefix + "SquashSpeed", 1 }, prefix + " Squash Speed",
            juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.45f));
    }
}

SpectralRotAudioProcessor::SpectralRotAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    for (auto& sample : outputWaveform)
        sample.store(0.0f, std::memory_order_relaxed);

    for (auto& waveform : compressorWaveforms)
        for (auto& sample : waveform)
            sample.store(0.0f, std::memory_order_relaxed);

    for (auto& writePosition : compressorWaveformWritePositions)
        writePosition.store(0, std::memory_order_relaxed);
}

juce::AudioProcessorValueTreeState::ParameterLayout SpectralRotAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { modeId, 1 }, "Mode", juce::StringArray { "Melt", "Rust", "Glass", "Squash" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "slot1Mode", 1 }, "Slot 1 Mode", juce::StringArray { "Melt", "Rust", "Glass", "Squash" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "slot2Mode", 1 }, "Slot 2 Mode", juce::StringArray { "Melt", "Rust", "Glass", "Squash" }, 1));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "slot3Mode", 1 }, "Slot 3 Mode", juce::StringArray { "Melt", "Rust", "Glass", "Squash" }, 2));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "slot4Mode", 1 }, "Slot 4 Mode", juce::StringArray { "Melt", "Rust", "Glass", "Squash" }, 3));

    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "slot1On", 1 }, "Slot 1 On", true));
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "slot2On", 1 }, "Slot 2 On", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "slot3On", 1 }, "Slot 3 On", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "slot4On", 1 }, "Slot 4 On", false));

    addModeParameters(params, ChainMode::melt, 0.0f, 0.58f, 0.25f, 0.62f, 90.0f);
    addModeParameters(params, ChainMode::rust, 0.55f, 0.58f, 0.0f, 0.0f, 20.0f);
    addModeParameters(params, ChainMode::glass, 0.35f, 0.35f, 0.0f, 0.0f, 90.0f);
    addModeParameters(params, ChainMode::squash, 0.0f, 0.0f, 0.5f, 0.45f, 90.0f);
    addSlotParameters(params, 1, 0.0f, 0.58f, 0.25f, 0.62f, 90.0f);
    addSlotParameters(params, 2, 0.55f, 0.58f, 0.0f, 0.0f, 20.0f);
    addSlotParameters(params, 3, 0.35f, 0.35f, 0.0f, 0.0f, 90.0f);
    addSlotParameters(params, 4, 0.0f, 0.0f, 0.5f, 0.45f, 90.0f);

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { mixId, 1 }, "Mix", juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 1.0f,
        percentAttributes()));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { outputId, 1 }, "Output", juce::NormalisableRange<float> { -24.0f, 12.0f, 0.01f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { widthId, 1 }, "Width", juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { inputGainId, 1 }, "Input Gain", juce::NormalisableRange<float> { -24.0f, 24.0f, 0.01f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { lowHoldId, 1 }, "High Pass", juce::NormalisableRange<float> { 20.0f, 1500.0f, 0.1f }, 90.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { lowPassId, 1 }, "Low Pass", juce::NormalisableRange<float> { 250.0f, 20000.0f, 0.1f }, 20000.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { glassReverseId, 1 }, "Reverse", juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { inputModeId, 1 }, "Input", juce::StringArray { "Stereo 1+2", "Input 1 Mono", "Input 2 Mono" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { harmonizerOnId, 1 }, "Harmonizer", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { harmonizerPitchId, 1 }, "Harmony Pitch", juce::NormalisableRange<float> { -12.0f, 12.0f, 0.01f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("st")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { harmonizerMixId, 1 }, "Harmony Mix", juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f,
        percentAttributes()));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { harmonizer2OnId, 1 }, "Harmonizer 2", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { harmonizer2PitchId, 1 }, "Harmony 2 Pitch", juce::NormalisableRange<float> { -12.0f, 12.0f, 0.01f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("st")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { harmonizer2MixId, 1 }, "Harmony 2 Mix", juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f,
        percentAttributes()));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { fftQualityId, 1 }, "FFT Quality", juce::StringArray { "Low 1024", "Normal 2048", "High 4096", "Ultra 8192" }, 1));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { compressorOnId, 1 }, "Spectral Compressor", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { compressorTameId, 1 }, "Compressor Tame",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { compressorThresholdId, 1 }, "Compressor Threshold",
        juce::NormalisableRange<float> { -48.0f, 0.0f, 0.01f }, -18.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { compressorFocusId, 1 }, "Compressor Focus",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { compressorSpeedId, 1 }, "Compressor Speed",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f }, 0.45f));

    return { params.begin(), params.end() };
}

void SpectralRotAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    for (auto& engine : engines)
        engine.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());

    for (auto& compressor : compressors)
        compressor.prepare(sampleRate, getTotalNumOutputChannels());
    dryDelayLines.assign(static_cast<size_t>(juce::jmax(1, getTotalNumOutputChannels())),
                         std::vector<float>(static_cast<size_t>(juce::jmax(1, static_cast<int>(sampleRate) * 2)), 0.0f));
    dryDelayPositions.assign(dryDelayLines.size(), 0);
    setLatencySamples(calculateReportedLatencySamples());
}

void SpectralRotAudioProcessor::releaseResources()
{
}

bool SpectralRotAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainOutput = layouts.getMainOutputChannelSet();

    if (mainOutput != juce::AudioChannelSet::mono() && mainOutput != juce::AudioChannelSet::stereo())
        return false;

    return mainOutput == layouts.getMainInputChannelSet();
}

void SpectralRotAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());

   #if JucePlugin_Build_Standalone
    applyInputMode(buffer);
   #endif
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer, true);

    updateEngineParameters();
    updateCompressorParameters();
    const auto wetLatencySamples = calculateReportedLatencySamples();
    delayDryBuffer(dryBuffer, wetLatencySamples);

    auto anySlotEnabled = false;
    auto globalInputApplied = false;

    for (int slot = 0; slot < static_cast<int>(engines.size()); ++slot)
    {
        const auto enabled = getParameterValue(parameters, "slot" + juce::String(slot + 1) + "On") > 0.5f;

        if (enabled)
        {
            if (! globalInputApplied)
            {
                engines.front().processGlobalInput(buffer);
                globalInputApplied = true;
            }

            anySlotEnabled = true;
            const auto mode = chainModeFromIndex(static_cast<int>(getParameterValue(parameters, "slot" + juce::String(slot + 1) + "Mode")));

            if (mode == ChainMode::squash)
            {
                compressors[static_cast<size_t>(slot)].process(buffer);
                pushCompressorActivity(slot, compressors[static_cast<size_t>(slot)].getLastGainReduction());
            }
            else
            {
                pushCompressorActivity(slot, 0.0f);
                engines[static_cast<size_t>(slot)].process(buffer);
            }
        }
        else
        {
            pushCompressorActivity(slot, 0.0f);
        }
    }

    if (anySlotEnabled)
    {
        engines.front().processGlobalOutput(buffer, dryBuffer);
    }
    else
    {
        engines.front().processGlobalInput(buffer);
        engines.front().processGlobalOutput(buffer, dryBuffer);
    }

    pushOutputWaveform(buffer);
}

void SpectralRotAudioProcessor::pushOutputWaveform(const juce::AudioBuffer<float>& buffer)
{
    const auto channels = juce::jmax(1, juce::jmin(2, buffer.getNumChannels()));
    auto write = outputWaveformWritePosition.load(std::memory_order_relaxed);

    for (int sample = 0; sample < buffer.getNumSamples(); sample += 2)
    {
        auto value = 0.0f;

        for (int channel = 0; channel < channels; ++channel)
            value += buffer.getSample(channel, sample);

        value /= static_cast<float>(channels);
        outputWaveform[static_cast<size_t>(write)].store(juce::jlimit(-1.0f, 1.0f, value), std::memory_order_relaxed);
        write = (write + 1) % static_cast<int>(outputWaveform.size());
    }

    outputWaveformWritePosition.store(write, std::memory_order_relaxed);
}

void SpectralRotAudioProcessor::copyOutputWaveform(std::array<float, 256>& destination) const
{
    const auto write = outputWaveformWritePosition.load(std::memory_order_relaxed);
    const auto stride = static_cast<int>(outputWaveform.size() / destination.size());
    auto read = write - static_cast<int>(outputWaveform.size());

    while (read < 0)
        read += static_cast<int>(outputWaveform.size());

    for (int i = 0; i < static_cast<int>(destination.size()); ++i)
    {
        auto peak = 0.0f;

        for (int j = 0; j < stride; ++j)
        {
            const auto index = (read + i * stride + j) % static_cast<int>(outputWaveform.size());
            const auto value = outputWaveform[static_cast<size_t>(index)].load(std::memory_order_relaxed);

            if (std::abs(value) > std::abs(peak))
                peak = value;
        }

        destination[static_cast<size_t>(i)] = peak;
    }
}

void SpectralRotAudioProcessor::pushCompressorActivity(int slot, float activity)
{
    if (! juce::isPositiveAndBelow(slot, static_cast<int>(compressorWaveforms.size())))
        return;

    auto& waveform = compressorWaveforms[static_cast<size_t>(slot)];
    auto& writePosition = compressorWaveformWritePositions[static_cast<size_t>(slot)];
    auto write = writePosition.load(std::memory_order_relaxed);
    waveform[static_cast<size_t>(write)].store(juce::jlimit(0.0f, 1.0f, activity), std::memory_order_relaxed);
    writePosition.store((write + 1) % static_cast<int>(waveform.size()), std::memory_order_relaxed);
}

void SpectralRotAudioProcessor::copyCompressorWaveform(int slot, std::array<float, 256>& destination) const
{
    if (! juce::isPositiveAndBelow(slot, static_cast<int>(compressorWaveforms.size())))
    {
        destination.fill(0.0f);
        return;
    }

    const auto& waveform = compressorWaveforms[static_cast<size_t>(slot)];
    const auto write = compressorWaveformWritePositions[static_cast<size_t>(slot)].load(std::memory_order_relaxed);
    const auto stride = static_cast<int>(waveform.size() / destination.size());
    auto read = write - static_cast<int>(waveform.size());

    while (read < 0)
        read += static_cast<int>(waveform.size());

    for (int i = 0; i < static_cast<int>(destination.size()); ++i)
    {
        auto peak = 0.0f;

        for (int j = 0; j < stride; ++j)
        {
            const auto index = (read + i * stride + j) % static_cast<int>(waveform.size());
            peak = juce::jmax(peak, waveform[static_cast<size_t>(index)].load(std::memory_order_relaxed));
        }

        destination[static_cast<size_t>(i)] = peak;
    }
}

void SpectralRotAudioProcessor::applyInputMode(juce::AudioBuffer<float>& buffer)
{
    if (getTotalNumInputChannels() < 2 || getTotalNumOutputChannels() < 2)
        return;

    switch (static_cast<int>(getParameterValue(parameters, inputModeId)))
    {
        case 1:
            buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());
            break;

        case 2:
            buffer.copyFrom(0, 0, buffer, 1, 0, buffer.getNumSamples());
            break;

        default:
            break;
    }
}

void SpectralRotAudioProcessor::delayDryBuffer(juce::AudioBuffer<float>& buffer, int delaySamples)
{
    const auto channels = juce::jmin(buffer.getNumChannels(), static_cast<int>(dryDelayLines.size()));

    if (channels <= 0 || delaySamples <= 0)
        return;

    const auto requiredSize = delaySamples + buffer.getNumSamples() + 1;

    for (int channel = 0; channel < channels; ++channel)
    {
        auto& line = dryDelayLines[static_cast<size_t>(channel)];

        if (static_cast<int>(line.size()) < requiredSize)
        {
            line.assign(static_cast<size_t>(requiredSize), 0.0f);
            dryDelayPositions[static_cast<size_t>(channel)] = 0;
        }

        auto position = dryDelayPositions[static_cast<size_t>(channel)];
        auto* data = buffer.getWritePointer(channel);
        const auto size = static_cast<int>(line.size());

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const auto readPosition = (position - delaySamples + size) % size;
            const auto input = data[sample];
            data[sample] = line[static_cast<size_t>(readPosition)];
            line[static_cast<size_t>(position)] = input;
            position = (position + 1) % size;
        }

        dryDelayPositions[static_cast<size_t>(channel)] = position;
    }
}

void SpectralRotAudioProcessor::updateEngineParameters()
{
    int firstActive = -1;
    int lastActive = -1;

    for (int slot = 0; slot < static_cast<int>(engines.size()); ++slot)
    {
        if (getParameterValue(parameters, "slot" + juce::String(slot + 1) + "On") > 0.5f)
        {
            if (firstActive < 0)
                firstActive = slot;

            lastActive = slot;
        }
    }

    const auto chainHasActiveSlot = firstActive >= 0;

    if (! chainHasActiveSlot)
        firstActive = lastActive = 0;

    for (int slot = 0; slot < static_cast<int>(engines.size()); ++slot)
    {
        const auto mode = chainModeFromIndex(static_cast<int>(getParameterValue(parameters, "slot" + juce::String(slot + 1) + "Mode")));
        const auto next = makeParametersForSlot(slot + 1, mode, slot == firstActive, slot == lastActive, chainHasActiveSlot);
        engines[static_cast<size_t>(slot)].setParameters(next);
    }

    setLatencySamples(calculateReportedLatencySamples());
}

void SpectralRotAudioProcessor::updateCompressorParameters()
{
    for (int slot = 0; slot < static_cast<int>(compressors.size()); ++slot)
        compressors[static_cast<size_t>(slot)].setParameters(makeCompressorParametersForSlot(slot + 1));
}

SpectralCompressor::Parameters SpectralRotAudioProcessor::makeCompressorParametersForSlot(int slot)
{
    const auto prefix = "slot" + juce::String(juce::jlimit(1, static_cast<int>(compressors.size()), slot));
    SpectralCompressor::Parameters next;
    next.enabled = true;
    next.tame = getParameterValue(parameters, prefix + "SquashTame");
    next.thresholdDb = getParameterValue(parameters, prefix + "SquashThreshold");
    next.focus = getParameterValue(parameters, prefix + "SquashFocus");
    next.speed = getParameterValue(parameters, prefix + "SquashSpeed");
    next.fftOrder = getFftOrder();
    return next;
}

int SpectralRotAudioProcessor::calculateReportedLatencySamples()
{
    auto spectralSlots = 0;

    for (int slot = 0; slot < static_cast<int>(engines.size()); ++slot)
    {
        const auto prefix = "slot" + juce::String(slot + 1);

        if (parameters.getRawParameterValue(prefix + "On")->load() <= 0.5f)
            continue;

        const auto mode = chainModeFromIndex(static_cast<int>(parameters.getRawParameterValue(prefix + "Mode")->load()));

        if (mode == ChainMode::squash)
            continue;

        if (mode == ChainMode::glass)
        {
            const auto texture = parameters.getRawParameterValue(prefix + "Lock")->load();
            const auto drift = std::abs(parameters.getRawParameterValue(prefix + "Tilt")->load());
            const auto reverse = parameters.getRawParameterValue(prefix + "Reverse")->load();
            const auto shatter = juce::jlimit(0.0f, 1.0f, (parameters.getRawParameterValue(prefix + "Shape")->load() - 20.0f) / 240.0f);

            if (std::abs(texture) + drift + reverse + shatter <= 0.001f)
                continue;
        }

        ++spectralSlots;
    }

    auto latency = engines.front().getLatencySamples() * spectralSlots;

    return latency;
}

SpectralRotEngine::Parameters SpectralRotAudioProcessor::makeParametersForSlot(int slot, ChainMode mode, bool firstActive, bool lastActive, bool chainHasActiveSlot)
{
    SpectralRotEngine::Parameters next;
    const auto prefix = "slot" + juce::String(juce::jlimit(1, static_cast<int>(engines.size()), slot));
    next.mode = engineModeFromChain(mode);
    next.drive = getParameterValue(parameters, prefix + "Drive");
    next.rot = getParameterValue(parameters, prefix + "Rot");
    next.lock = getParameterValue(parameters, prefix + "Lock");
    next.tilt = getParameterValue(parameters, prefix + "Tilt");
    next.subGuardHz = getParameterValue(parameters, prefix + "Shape");
    next.mix = getParameterValue(parameters, mixId);
    next.outputGain = juce::Decibels::decibelsToGain(getParameterValue(parameters, outputId));
    next.width = getParameterValue(parameters, widthId);
    next.inputGain = juce::Decibels::decibelsToGain(getParameterValue(parameters, inputGainId));
    next.lowHoldHz = getParameterValue(parameters, lowHoldId);
    next.lowPassHz = getParameterValue(parameters, lowPassId);
    next.glassReverse = getParameterValue(parameters, prefix + "Reverse");
    next.glassMix = getParameterValue(parameters, prefix + "GlassMix");
    next.edgeBeforeCorrode = getParameterValue(parameters, prefix + "EdgePre") > 0.5f;
    next.harmonizerEnabled = getParameterValue(parameters, harmonizerOnId) > 0.5f;
    next.harmonizerPitchSemitones = getParameterValue(parameters, harmonizerPitchId);
    next.harmonizerMix = getParameterValue(parameters, harmonizerMixId);
    next.harmonizer2Enabled = getParameterValue(parameters, harmonizer2OnId) > 0.5f;
    next.harmonizer2PitchSemitones = getParameterValue(parameters, harmonizer2PitchId);
    next.harmonizer2Mix = getParameterValue(parameters, harmonizer2MixId);
    next.fftOrder = getFftOrder();
    next.applyInputGain = ! chainHasActiveSlot && firstActive;
    next.applyHarmonizer = ! chainHasActiveSlot && firstActive;
    next.applyOutputGain = ! chainHasActiveSlot && lastActive;
    next.applyWidth = ! chainHasActiveSlot && lastActive;
    return next;
}

int SpectralRotAudioProcessor::getFftOrder() const
{
    switch (static_cast<int>(parameters.getRawParameterValue(fftQualityId)->load()))
    {
        case 0: return 10;
        case 2: return 12;
        case 3: return 13;
        default: return 11;
    }
}

juce::AudioProcessorEditor* SpectralRotAudioProcessor::createEditor()
{
    return new SpectralRotAudioProcessorEditor(*this);
}

bool SpectralRotAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String SpectralRotAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SpectralRotAudioProcessor::acceptsMidi() const
{
    return false;
}

bool SpectralRotAudioProcessor::producesMidi() const
{
    return false;
}

bool SpectralRotAudioProcessor::isMidiEffect() const
{
    return false;
}

double SpectralRotAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SpectralRotAudioProcessor::getNumPrograms()
{
    return 1;
}

int SpectralRotAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SpectralRotAudioProcessor::setCurrentProgram(int)
{
}

const juce::String SpectralRotAudioProcessor::getProgramName(int)
{
    return {};
}

void SpectralRotAudioProcessor::changeProgramName(int, const juce::String&)
{
}

void SpectralRotAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    removeParameterFromState(state, inputModeId);
    juce::MemoryOutputStream stream(destData, true);
    state.writeToStream(stream);
}

void SpectralRotAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    const auto currentInputMode = getParameterValue(parameters, inputModeId);
    const auto tree = juce::ValueTree::readFromData(data, static_cast<size_t>(sizeInBytes));

    if (tree.isValid())
    {
        parameters.replaceState(tree);
        setParameterValue(parameters, inputModeId, currentInputMode);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpectralRotAudioProcessor();
}
