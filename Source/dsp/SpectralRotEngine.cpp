#include "dsp/SpectralRotEngine.h"

namespace
{
    constexpr float pi = juce::MathConstants<float>::pi;

    float clamp01(float value)
    {
        return juce::jlimit(0.0f, 1.0f, value);
    }

    float safeMagnitude(float real, float imag)
    {
        return std::sqrt((real * real) + (imag * imag) + 1.0e-12f);
    }
}

void SpectralRotEngine::prepare(double newSampleRate, int, int channels)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    currentChannels = juce::jmax(1, channels);
    configureFft(currentFftOrder, currentChannels);
    prepareWidthDelay();
}

void SpectralRotEngine::configureFft(int newFftOrder, int channels)
{
    currentFftOrder = juce::jlimit(10, 13, newFftOrder);
    fftSize = 1 << currentFftOrder;
    hopSize = fftSize / overlap;
    numBins = (fftSize / 2) + 1;
    fft = std::make_unique<juce::dsp::FFT>(currentFftOrder);
    window = std::make_unique<juce::dsp::WindowingFunction<float>>(static_cast<size_t>(fftSize), juce::dsp::WindowingFunction<float>::hann, false);
    channelStates.resize(static_cast<size_t>(juce::jmax(1, channels)));

    for (int channel = 0; channel < static_cast<int>(channelStates.size()); ++channel)
    {
        allocateChannelState(channelStates[static_cast<size_t>(channel)]);
        channelStates[static_cast<size_t>(channel)].stereoSign = (channel % 2 == 0) ? -1.0f : 1.0f;
    }
}

void SpectralRotEngine::allocateChannelState(ChannelState& state)
{
    state.inputRing.assign(static_cast<size_t>(fftSize), 0.0f);
    state.outputRing.assign(static_cast<size_t>(fftSize), 0.0f);
    state.fftData.assign(static_cast<size_t>(fftSize * 2), 0.0f);
    state.originalMagnitudes.assign(static_cast<size_t>(numBins), 0.0f);
    state.processedMagnitudes.assign(static_cast<size_t>(numBins), 0.0f);
    state.spectralAdditions.assign(static_cast<size_t>(numBins), 0.0f);
    state.glassFeedbackReal.assign(static_cast<size_t>(maxSpectralDelayFrames * numBins), 0.0f);
    state.glassFeedbackImag.assign(static_cast<size_t>(maxSpectralDelayFrames * numBins), 0.0f);
    state.glassDelay.assign(static_cast<size_t>(juce::jmax(4096, static_cast<int>(sampleRate * 2.5))), 0.0f);
    state.spectralHistoryReal.assign(static_cast<size_t>(maxSpectralDelayFrames * numBins), 0.0f);
    state.spectralHistoryImag.assign(static_cast<size_t>(maxSpectralDelayFrames * numBins), 0.0f);
    state.harmonizerDelay.assign(static_cast<size_t>(juce::jmax(4096, static_cast<int>(sampleRate * 0.18))), 0.0f);
    state.harmonizer2Delay.assign(static_cast<size_t>(juce::jmax(4096, static_cast<int>(sampleRate * 0.18))), 0.0f);
    state.writePosition = 0;
    state.harmonizerWritePosition = 0;
    state.harmonizer2WritePosition = 0;
    state.glassDelayWritePosition = 0;
    state.spectralHistoryWritePosition = 0;
    state.hopCounter = 0;
    state.frameCounter = 0;
    state.stereoSign = 1.0f;
    state.harmonizerPhaseA = 0.0f;
    state.harmonizerPhaseB = 0.5f;
    state.harmonizer2PhaseA = 0.0f;
    state.harmonizer2PhaseB = 0.5f;
    state.phase = 0.0f;
    state.slowPhase = 0.0f;
    state.rmsCompensation = 1.0f;
    state.corrodeEnvelope = 0.0f;
    state.pink0 = 0.0f;
    state.pink1 = 0.0f;
    state.pink2 = 0.0f;
    state.lowPassY1 = 0.0f;
    state.rustCrushHeldSample = 0.0f;
    state.highPassX1 = 0.0f;
    state.highPassY1 = 0.0f;
    state.phaserX.fill(0.0f);
    state.phaserY.fill(0.0f);
    state.vowelBand.fill(0.0f);
    state.vowelLow.fill(0.0f);
    state.phaserFeedback = 0.0f;
    state.ringPhase = 0.0f;
    state.ringEnvelope = 0.0f;
    state.ringLowpass = 0.0f;
    state.lastRingInput = 0.0f;
    state.ringPeriodSamples = 96.0f;
    state.ringZeroCrossCounter = 0;
    state.rustCrushCounter = 0;
    state.noiseState = 0x12345678u;
}

void SpectralRotEngine::reset()
{
    for (auto& state : channelStates)
    {
        std::fill(state.inputRing.begin(), state.inputRing.end(), 0.0f);
        std::fill(state.outputRing.begin(), state.outputRing.end(), 0.0f);
        std::fill(state.fftData.begin(), state.fftData.end(), 0.0f);
        std::fill(state.originalMagnitudes.begin(), state.originalMagnitudes.end(), 0.0f);
        std::fill(state.processedMagnitudes.begin(), state.processedMagnitudes.end(), 0.0f);
        std::fill(state.spectralAdditions.begin(), state.spectralAdditions.end(), 0.0f);
        std::fill(state.glassFeedbackReal.begin(), state.glassFeedbackReal.end(), 0.0f);
        std::fill(state.glassFeedbackImag.begin(), state.glassFeedbackImag.end(), 0.0f);
        std::fill(state.glassDelay.begin(), state.glassDelay.end(), 0.0f);
        std::fill(state.spectralHistoryReal.begin(), state.spectralHistoryReal.end(), 0.0f);
        std::fill(state.spectralHistoryImag.begin(), state.spectralHistoryImag.end(), 0.0f);
        std::fill(state.harmonizerDelay.begin(), state.harmonizerDelay.end(), 0.0f);
        std::fill(state.harmonizer2Delay.begin(), state.harmonizer2Delay.end(), 0.0f);
        state.writePosition = 0;
        state.harmonizerWritePosition = 0;
        state.harmonizer2WritePosition = 0;
        state.glassDelayWritePosition = 0;
        state.spectralHistoryWritePosition = 0;
        state.hopCounter = 0;
        state.frameCounter = 0;
        state.harmonizerPhaseA = 0.0f;
        state.harmonizerPhaseB = 0.5f;
        state.harmonizer2PhaseA = 0.0f;
        state.harmonizer2PhaseB = 0.5f;
        state.phase = 0.0f;
        state.slowPhase = 0.0f;
        state.rmsCompensation = 1.0f;
        state.corrodeEnvelope = 0.0f;
        state.pink0 = 0.0f;
        state.pink1 = 0.0f;
        state.pink2 = 0.0f;
        state.lowPassY1 = 0.0f;
        state.rustCrushHeldSample = 0.0f;
        state.highPassX1 = 0.0f;
        state.highPassY1 = 0.0f;
        state.phaserX.fill(0.0f);
        state.phaserY.fill(0.0f);
        state.vowelBand.fill(0.0f);
        state.vowelLow.fill(0.0f);
        state.phaserFeedback = 0.0f;
        state.ringPhase = 0.0f;
        state.ringEnvelope = 0.0f;
        state.ringLowpass = 0.0f;
        state.lastRingInput = 0.0f;
        state.ringPeriodSamples = 96.0f;
        state.ringZeroCrossCounter = 0;
        state.rustCrushCounter = 0;
        state.noiseState = 0x12345678u;
    }

    std::fill(widthDelay.begin(), widthDelay.end(), 0.0f);
    widthWritePosition = 0;
    widthPhase = 0.0f;
}

void SpectralRotEngine::setParameters(const Parameters& newParameters)
{
    if (newParameters.fftOrder != currentFftOrder)
        configureFft(newParameters.fftOrder, currentChannels);

    parameters = newParameters;
}

int SpectralRotEngine::getLatencySamples() const noexcept
{
    return fftSize - hopSize;
}

void SpectralRotEngine::process(juce::AudioBuffer<float>& buffer)
{
    const auto channels = juce::jmin(buffer.getNumChannels(), static_cast<int>(channelStates.size()));
    const auto samples = buffer.getNumSamples();

    for (int channel = 0; channel < channels; ++channel)
    {
        auto* data = buffer.getWritePointer(channel);
        auto& state = channelStates[static_cast<size_t>(channel)];

        for (int sample = 0; sample < samples; ++sample)
        {
            const auto input = data[sample] * (parameters.applyInputGain ? parameters.inputGain : 1.0f);
            auto harmonized = parameters.applyHarmonizer ? processHarmonizer(state, input, false) : input;
            harmonized = parameters.applyHarmonizer ? processHarmonizer(state, harmonized, true) : harmonized;
            float wet = 0.0f;

            if (parameters.mode == Mode::glass)
            {
                const auto delayed = processGlassDelaySample(state, harmonized);
                const auto spectralAmount = juce::jlimit(0.0f, 1.0f,
                    std::abs(parameters.lock) + std::abs(parameters.tilt) + parameters.glassReverse
                    + juce::jlimit(0.0f, 1.0f, (parameters.subGuardHz - 20.0f) / 240.0f));

                if (spectralAmount > 0.001f)
                {
                    float spectralWet = 0.0f;
                    processSample(state, delayed, spectralWet);
                    wet = juce::jmap(juce::jlimit(0.0f, 1.0f, spectralAmount), delayed, spectralWet);
                }
                else
                {
                    wet = delayed;
                }

                wet = juce::jmap(juce::jlimit(0.0f, 1.0f, parameters.glassMix), harmonized, wet);
            }
            else
            {
                processSample(state, harmonized, wet);
            }

            if (parameters.mode == Mode::rust)
            {
                if (parameters.edgeBeforeCorrode)
                    wet = processRustPhaser(state, wet);
                wet = processRustCorrodeSample(state, wet);
                if (! parameters.edgeBeforeCorrode)
                    wet = processRustPhaser(state, wet);
                wet = processGonkRing(state, wet, clamp01((parameters.subGuardHz - 20.0f) / 240.0f));
                wet = processRustBitcrush(state, wet);
            }
            data[sample] = wet;
        }
    }

    if (parameters.applyWidth)
        applyWidth(buffer, channels, samples);
}

void SpectralRotEngine::processGlobalInput(juce::AudioBuffer<float>& buffer)
{
    const auto channels = juce::jmin(buffer.getNumChannels(), static_cast<int>(channelStates.size()));
    const auto samples = buffer.getNumSamples();

    for (int channel = 0; channel < channels; ++channel)
    {
        auto* data = buffer.getWritePointer(channel);
        auto& state = channelStates[static_cast<size_t>(channel)];

        for (int sample = 0; sample < samples; ++sample)
        {
            const auto dry = data[sample];
            auto wet = dry * parameters.inputGain;
            wet = processHarmonizer(state, wet, false);
            wet = processHarmonizer(state, wet, true);
            data[sample] = wet;
        }
    }
}

void SpectralRotEngine::processGlobalBus(juce::AudioBuffer<float>& buffer)
{
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer, true);

    processGlobalInput(buffer);
    processGlobalOutput(buffer, dryBuffer);
}

void SpectralRotEngine::processGlobalOutput(juce::AudioBuffer<float>& wetBuffer, const juce::AudioBuffer<float>& dryBuffer)
{
    const auto channels = juce::jmin(juce::jmin(wetBuffer.getNumChannels(), dryBuffer.getNumChannels()),
                                     static_cast<int>(channelStates.size()));
    const auto samples = juce::jmin(wetBuffer.getNumSamples(), dryBuffer.getNumSamples());
    const auto mix = juce::jlimit(0.0f, 1.0f, parameters.mix);

    for (int channel = 0; channel < channels; ++channel)
    {
        auto* wet = wetBuffer.getWritePointer(channel);
        auto& state = channelStates[static_cast<size_t>(channel)];

        for (int sample = 0; sample < samples; ++sample)
        {
            wet[sample] = processHighPass(state, wet[sample]);
            wet[sample] = processLowPass(state, wet[sample]);
        }
    }

    applyWidth(wetBuffer, channels, samples);

    for (int channel = 0; channel < channels; ++channel)
    {
        auto* wet = wetBuffer.getWritePointer(channel);
        const auto* dry = dryBuffer.getReadPointer(channel);

        for (int sample = 0; sample < samples; ++sample)
            wet[sample] = ((dry[sample] * (1.0f - mix)) + (wet[sample] * mix)) * parameters.outputGain;
    }
}

void SpectralRotEngine::applyWidth(juce::AudioBuffer<float>& buffer, int channels, int samples)
{
    if (channels >= 2 && parameters.width > 0.001f)
    {
        auto* left = buffer.getWritePointer(0);
        auto* right = buffer.getWritePointer(1);
        const auto width = parameters.width;
        const auto sideGain = 1.0f + width * 1.15f;
        const auto decorrelationGain = width * (0.12f + width * 0.24f);
        const auto phaseStep = (juce::MathConstants<float>::twoPi * (0.08f + width * 0.42f)) / static_cast<float>(juce::jmax(1.0, sampleRate));

        for (int sample = 0; sample < samples; ++sample)
        {
            const auto mid = (left[sample] + right[sample]) * 0.5f;
            const auto existingSide = (left[sample] - right[sample]) * 0.5f;
            const auto wobble = (std::sin(widthPhase) + 1.0f) * 0.5f;
            const auto shortDelay = (0.0035f + wobble * 0.0035f) * static_cast<float>(sampleRate);
            const auto longDelay = (0.011f + (1.0f - wobble) * 0.009f) * static_cast<float>(sampleRate);
            const auto decorrelatedSide = std::tanh((readWidthDelay(shortDelay) - readWidthDelay(longDelay)) * (0.7f + width * 1.4f));
            const auto side = (existingSide * sideGain) + decorrelatedSide * decorrelationGain;

            widthDelay[static_cast<size_t>(widthWritePosition)] = mid;
            widthWritePosition = (widthWritePosition + 1) % static_cast<int>(widthDelay.size());
            widthPhase += phaseStep;

            if (widthPhase >= juce::MathConstants<float>::twoPi)
                widthPhase -= juce::MathConstants<float>::twoPi;

            left[sample] = mid + side;
            right[sample] = mid - side;
        }
    }
}

float SpectralRotEngine::processGlassDelaySample(ChannelState& state, float input) const
{
    if (state.glassDelay.empty())
        return input;

    const auto size = static_cast<int>(state.glassDelay.size());
    const auto delaySeconds = 0.035f + std::pow(juce::jlimit(0.0f, 1.0f, parameters.drive), 1.55f) * 1.45f;
    const auto delaySamples = juce::jlimit(1.0f, static_cast<float>(size - 4), delaySeconds * static_cast<float>(sampleRate));
    auto readPosition = static_cast<float>(state.glassDelayWritePosition) - delaySamples;

    while (readPosition < 0.0f)
        readPosition += static_cast<float>(size);

    const auto index0 = static_cast<int>(readPosition) % size;
    const auto index1 = (index0 + 1) % size;
    const auto fraction = readPosition - std::floor(readPosition);
    const auto delayed = state.glassDelay[static_cast<size_t>(index0)] * (1.0f - fraction)
                       + state.glassDelay[static_cast<size_t>(index1)] * fraction;
    const auto feedback = parameters.rot >= 0.999f
        ? 0.9995f
        : juce::jlimit(0.0f, 0.96f, parameters.rot * 0.94f);

    state.glassDelay[static_cast<size_t>(state.glassDelayWritePosition)] = input + delayed * feedback;
    state.glassDelayWritePosition = (state.glassDelayWritePosition + 1) % size;

    return delayed;
}

float SpectralRotEngine::processRustBitcrush(ChannelState& state, float input) const
{
    const auto crush = juce::jlimit(0.0f, 0.9f, parameters.lock * 0.9f);

    if (crush <= 0.001f)
        return input;

    const auto holdSamples = 1 + static_cast<int>(std::round(crush * crush * 38.0f));
    if (state.rustCrushCounter <= 0)
    {
        const auto bits = juce::jlimit(3.0f, 16.0f, 16.0f - crush * 13.0f);
        const auto levels = std::pow(2.0f, bits);
        state.rustCrushHeldSample = std::round(input * levels) / levels;
        state.rustCrushCounter = holdSamples;
    }

    --state.rustCrushCounter;
    return juce::jmap(crush, input, state.rustCrushHeldSample);
}

float SpectralRotEngine::processRustCorrodeSample(ChannelState& state, float input)
{
    const auto corrode = clamp01(parameters.drive);
    const auto envelopeTarget = std::abs(input);
    const auto envelopeRate = envelopeTarget > state.corrodeEnvelope ? 0.05f : 0.006f;
    state.corrodeEnvelope += (envelopeTarget - state.corrodeEnvelope) * envelopeRate;

    if (corrode <= 0.001f)
        return input;

    const auto white = (nextNoise(state) * 2.0f) - 1.0f;
    state.pink0 = 0.99765f * state.pink0 + white * 0.0990460f;
    state.pink1 = 0.96300f * state.pink1 + white * 0.2965164f;
    state.pink2 = 0.57000f * state.pink2 + white * 1.0526913f;
    const auto pink = (state.pink0 + state.pink1 + state.pink2 + white * 0.1848f) * 0.08f;
    const auto gate = juce::jlimit(0.0f, 1.0f, state.corrodeEnvelope * 8.0f);
    const auto keyedNoise = pink * gate * std::pow(corrode, 0.72f) * (0.012f + state.corrodeEnvelope * 0.72f);
    const auto noisy = input + keyedNoise;
    const auto crush = noisy * (1.0f + corrode * 46.0f);
    const auto driven = std::tanh(crush) + 0.42f * std::tanh((noisy + keyedNoise * 0.75f) * (1.0f + corrode * 70.0f));
    const auto grit = driven - std::tanh(input * (1.0f + corrode * 5.5f)) * 0.11f;
    return juce::jmap(std::pow(corrode, 0.7f), input, grit * (0.58f + corrode * 0.2f));
}

float SpectralRotEngine::processRustPhaser(ChannelState& state, float input)
{
    const auto edge = clamp01(parameters.tilt);

    if (edge <= 0.001f)
        return input;

    const float vowels[5][4] = {
        { 800.0f, 1150.0f, 2900.0f, 3900.0f },
        { 350.0f, 1700.0f, 2700.0f, 3700.0f },
        { 450.0f,  900.0f, 2400.0f, 3400.0f },
        { 325.0f,  700.0f, 2530.0f, 3500.0f },
        { 600.0f, 1000.0f, 2600.0f, 3800.0f }
    };

    const auto vowelPosition = edge * 4.0f;
    const auto vowelIndex = juce::jlimit(0, 3, static_cast<int>(std::floor(vowelPosition)));
    const auto vowelBlend = vowelPosition - static_cast<float>(vowelIndex);
    auto resonant = 0.0f;

    for (int band = 0; band < 4; ++band)
    {
        const auto formant = juce::jmap(vowelBlend, vowels[vowelIndex][band], vowels[vowelIndex + 1][band]);
        const auto f = 2.0f * std::sin(pi * juce::jlimit(40.0f, static_cast<float>(sampleRate) * 0.45f, formant) / static_cast<float>(sampleRate));
        const auto q = 0.045f + edge * 0.018f + static_cast<float>(band) * 0.004f;
        state.vowelLow[static_cast<size_t>(band)] += f * state.vowelBand[static_cast<size_t>(band)];
        const auto high = input - state.vowelLow[static_cast<size_t>(band)] - q * state.vowelBand[static_cast<size_t>(band)];
        state.vowelBand[static_cast<size_t>(band)] += f * high;
        const auto weight = band == 0 ? 0.9f : (band == 1 ? 0.72f : 0.38f);
        resonant += state.vowelBand[static_cast<size_t>(band)] * weight;
    }

    const auto wet = std::tanh((input * (0.82f - edge * 0.24f) + resonant * (0.55f + edge * 1.25f)) * (1.0f + edge * 0.55f));
    return juce::jmap(edge, input, wet);
}

float SpectralRotEngine::processGonkRing(ChannelState& state, float input, float amount)
{
    const auto ring = juce::jlimit(0.0f, 1.0f, amount);
    const auto envelopeTarget = std::abs(input);
    const auto envelopeRate = envelopeTarget > state.ringEnvelope ? 0.035f : 0.0025f;
    state.ringEnvelope += (envelopeTarget - state.ringEnvelope) * envelopeRate;
    state.ringLowpass += (input - state.ringLowpass) * (0.012f + ring * 0.026f);

    if (ring <= 0.001f)
    {
        state.lastRingInput = input;
        return input;
    }

    ++state.ringZeroCrossCounter;
    const auto crossedUp = state.lastRingInput <= 0.0f && input > 0.0f;
    const auto minPeriod = static_cast<int>(sampleRate / 950.0);
    const auto maxPeriod = static_cast<int>(sampleRate / 24.0);

    if (crossedUp && state.ringZeroCrossCounter >= minPeriod)
    {
        const auto measured = juce::jlimit(static_cast<float>(minPeriod), static_cast<float>(maxPeriod), static_cast<float>(state.ringZeroCrossCounter));
        state.ringPeriodSamples = state.ringPeriodSamples * 0.985f + measured * 0.015f;
        state.ringZeroCrossCounter = 0;
    }

    if (state.ringZeroCrossCounter > maxPeriod)
        state.ringZeroCrossCounter = maxPeriod;

    state.lastRingInput = input;

    const auto octaveDivider = 2.0f + ring * 6.0f;
    const auto period = juce::jlimit(static_cast<float>(minPeriod), static_cast<float>(maxPeriod) * 8.0f,
                                    state.ringPeriodSamples * octaveDivider);
    state.ringPhase += juce::MathConstants<float>::twoPi / period;

    if (state.ringPhase >= juce::MathConstants<float>::twoPi)
        state.ringPhase -= juce::MathConstants<float>::twoPi;

    const auto sine = std::sin(state.ringPhase);
    const auto triangle = (2.0f / pi) * std::asin(sine);
    const auto selfShape = std::tanh(state.ringLowpass * (2.0f + ring * 6.0f));
    const auto carrier = std::tanh((triangle * (0.78f - ring * 0.18f) + selfShape * (0.25f + ring * 0.42f)) * (1.0f + ring * 1.25f));
    const auto diodeInput = std::tanh(input * (1.0f + ring * 2.8f));
    const auto ringed = diodeInput * carrier;
    const auto rectifiedCrunch = std::tanh((ringed + std::abs(state.ringLowpass) * ring * 0.22f) * (1.15f + ring * 1.7f));
    const auto wet = juce::jmap(ring * 0.45f, ringed, rectifiedCrunch) * (0.92f / (1.0f + ring * 0.75f));

    return juce::jmap(std::pow(ring, 0.72f), input, wet);
}

void SpectralRotEngine::prepareWidthDelay()
{
    const auto delaySamples = juce::jmax(1024, static_cast<int>(sampleRate * 0.08));
    widthDelay.assign(static_cast<size_t>(delaySamples), 0.0f);
    widthWritePosition = 0;
    widthPhase = 0.0f;
}

float SpectralRotEngine::readWidthDelay(float delaySamples) const
{
    if (widthDelay.empty())
        return 0.0f;

    const auto size = static_cast<int>(widthDelay.size());
    auto readPosition = static_cast<float>(widthWritePosition) - delaySamples;

    while (readPosition < 0.0f)
        readPosition += static_cast<float>(size);

    const auto index0 = static_cast<int>(readPosition) % size;
    const auto index1 = (index0 + 1) % size;
    const auto fraction = readPosition - std::floor(readPosition);

    return widthDelay[static_cast<size_t>(index0)] * (1.0f - fraction)
         + widthDelay[static_cast<size_t>(index1)] * fraction;
}

void SpectralRotEngine::processSample(ChannelState& state, float input, float& output)
{
    state.inputRing[static_cast<size_t>(state.writePosition)] = input;

    output = state.outputRing[static_cast<size_t>(state.writePosition)];
    state.outputRing[static_cast<size_t>(state.writePosition)] = 0.0f;

    state.writePosition = (state.writePosition + 1) % fftSize;

    if (++state.hopCounter >= hopSize)
    {
        state.hopCounter = 0;
        processFrame(state);
    }
}

float SpectralRotEngine::processHarmonizer(ChannelState& state, float input, bool second)
{
    auto& delay = second ? state.harmonizer2Delay : state.harmonizerDelay;
    auto& writePosition = second ? state.harmonizer2WritePosition : state.harmonizerWritePosition;
    auto& phaseA = second ? state.harmonizer2PhaseA : state.harmonizerPhaseA;
    auto& phaseB = second ? state.harmonizer2PhaseB : state.harmonizerPhaseB;
    const auto enabled = second ? parameters.harmonizer2Enabled : parameters.harmonizerEnabled;
    const auto mix = juce::jlimit(0.0f, 1.0f, second ? parameters.harmonizer2Mix : parameters.harmonizerMix);
    const auto pitch = second ? parameters.harmonizer2PitchSemitones : parameters.harmonizerPitchSemitones;

    if (delay.empty())
        return input;

    delay[static_cast<size_t>(writePosition)] = input;

    if (! enabled || mix <= 0.001f || std::abs(pitch) < 0.01f)
    {
        writePosition = (writePosition + 1) % static_cast<int>(delay.size());
        return input;
    }

    const auto ratio = std::pow(2.0f, pitch / 12.0f);
    const auto windowSamples = juce::jlimit(1024.0f, static_cast<float>(delay.size() - 4), static_cast<float>(sampleRate) * 0.085f);
    const auto phaseIncrement = std::abs(ratio - 1.0f) / windowSamples;

    auto renderHead = [&] (float phase)
    {
        const auto delay = ratio >= 1.0f ? (1.0f - phase) * windowSamples : phase * windowSamples;
        const auto window = std::sin(phase * pi);
        return readHarmonizerDelay(state, delay, second) * window;
    };

    const auto shifted = (renderHead(phaseA) + renderHead(phaseB)) * 0.85f;
    phaseA += phaseIncrement;
    phaseB += phaseIncrement;

    if (phaseA >= 1.0f)
        phaseA -= 1.0f;

    if (phaseB >= 1.0f)
        phaseB -= 1.0f;

    writePosition = (writePosition + 1) % static_cast<int>(delay.size());
    return input + shifted * mix;
}

float SpectralRotEngine::readHarmonizerDelay(const ChannelState& state, float delaySamples, bool second) const
{
    const auto& delay = second ? state.harmonizer2Delay : state.harmonizerDelay;
    const auto writePosition = second ? state.harmonizer2WritePosition : state.harmonizerWritePosition;
    const auto size = static_cast<int>(delay.size());
    auto readPosition = static_cast<float>(writePosition) - delaySamples;

    while (readPosition < 0.0f)
        readPosition += static_cast<float>(size);

    const auto index0 = static_cast<int>(readPosition) % size;
    const auto index1 = (index0 + 1) % size;
    const auto fraction = readPosition - std::floor(readPosition);
    return delay[static_cast<size_t>(index0)] * (1.0f - fraction)
         + delay[static_cast<size_t>(index1)] * fraction;
}

float SpectralRotEngine::processHighPass(ChannelState& state, float input) const
{
    const auto cutoff = juce::jlimit(20.0f, 1500.0f, parameters.lowHoldHz);
    const auto rc = 1.0f / (juce::MathConstants<float>::twoPi * cutoff);
    const auto dt = 1.0f / static_cast<float>(sampleRate);
    const auto alpha = rc / (rc + dt);
    const auto output = alpha * (state.highPassY1 + input - state.highPassX1);
    state.highPassX1 = input;
    state.highPassY1 = output;
    return output;
}

float SpectralRotEngine::processLowPass(ChannelState& state, float input) const
{
    const auto cutoff = juce::jlimit(250.0f, 20000.0f, parameters.lowPassHz);
    const auto rc = 1.0f / (juce::MathConstants<float>::twoPi * cutoff);
    const auto dt = 1.0f / static_cast<float>(sampleRate);
    const auto alpha = dt / (rc + dt);
    state.lowPassY1 += alpha * (input - state.lowPassY1);
    return state.lowPassY1;
}

bool SpectralRotEngine::hasGlassTail(const ChannelState& state) const
{
    if (parameters.mode != Mode::glass)
        return false;

    auto energy = 0.0f;

    for (int frame = 0; frame < maxSpectralDelayFrames; frame += 8)
    {
        for (int bin = 1; bin < numBins - 1; bin += 16)
        {
            const auto index = static_cast<size_t>(frame * numBins + bin);
            const auto real = state.glassFeedbackReal[index];
            const auto imag = state.glassFeedbackImag[index];
            energy += real * real + imag * imag;
        }
    }

    return energy > 1.0e-8f;
}

int SpectralRotEngine::spectralHistoryIndex(int frameOffset, int bin, int writePosition) const
{
    const auto frame = (writePosition - frameOffset + maxSpectralDelayFrames) % maxSpectralDelayFrames;
    return frame * numBins + juce::jlimit(0, numBins - 1, bin);
}

int SpectralRotEngine::glassDelayIndex(int frameOffset, int bin, int writePosition) const
{
    const auto frame = (writePosition - frameOffset + maxSpectralDelayFrames) % maxSpectralDelayFrames;
    return frame * numBins + juce::jlimit(0, numBins - 1, bin);
}

void SpectralRotEngine::processFrame(ChannelState& state)
{
    std::fill(state.fftData.begin(), state.fftData.end(), 0.0f);
    for (int i = 0; i < fftSize; ++i)
    {
        const auto ringIndex = (state.writePosition + i) % fftSize;
        state.fftData[static_cast<size_t>(i)] = state.inputRing[static_cast<size_t>(ringIndex)];
    }

    window->multiplyWithWindowingTable(state.fftData.data(), static_cast<size_t>(fftSize));

    fft->performRealOnlyForwardTransform(state.fftData.data(), true);

    for (int bin = 0; bin < numBins; ++bin)
    {
        const auto realIndex = static_cast<size_t>(bin * 2);
        const auto imagIndex = realIndex + 1;
        const auto historyIndex = static_cast<size_t>(state.spectralHistoryWritePosition * numBins + bin);
        state.spectralHistoryReal[historyIndex] = state.fftData[realIndex];
        state.spectralHistoryImag[historyIndex] = state.fftData[imagIndex];
    }

    for (int bin = 0; bin < numBins; ++bin)
    {
        const auto realIndex = static_cast<size_t>(bin * 2);
        const auto imagIndex = realIndex + 1;
        state.originalMagnitudes[static_cast<size_t>(bin)] = safeMagnitude(state.fftData[realIndex], state.fftData[imagIndex]);
        state.processedMagnitudes[static_cast<size_t>(bin)] = state.originalMagnitudes[static_cast<size_t>(bin)];
    }
    std::fill(state.spectralAdditions.begin(), state.spectralAdditions.end(), 0.0f);

    const auto rot = parameters.rot;
    const auto tilt = parameters.tilt;
    const auto frameRate = static_cast<float>(sampleRate) / static_cast<float>(hopSize);
    if (parameters.mode == Mode::rust)
        state.phase += (juce::MathConstants<float>::twoPi * 0.18f) / juce::jmax(1.0f, frameRate);
    else
        state.phase += 0.05f + (rot * 0.45f);

    state.slowPhase += (0.12f + rot * 2.6f + parameters.lock * 2.4f) / juce::jmax(1.0f, frameRate);
    ++state.frameCounter;

    for (int bin = 1; bin < numBins - 1; ++bin)
    {
        const auto original = state.originalMagnitudes[static_cast<size_t>(bin)];
        const auto envelope = calculateLocalAverage(state.originalMagnitudes, bin);
        const auto binNorm = static_cast<float>(bin) / static_cast<float>(numBins - 1);
        auto processed = original;

        if (parameters.mode == Mode::melt)
        {
            const auto morph = juce::jlimit(-1.0f, 1.0f, parameters.drive);
            const auto phaserAmount = std::max(0.0f, -morph);
            const auto flangerAmount = std::max(0.0f, morph);
            const auto bandSpread = parameters.rot;
            const auto motion = parameters.lock;
            const auto depth = 0.24f + clamp01(parameters.tilt) * 1.18f;
            const auto travel = state.slowPhase * (0.2f + motion * 11.0f);
            const auto frequency = (static_cast<float>(bin) * static_cast<float>(sampleRate)) / static_cast<float>(fftSize);
            const auto lowEdge = juce::jmap(bandSpread, 400.0f, 20.0f);
            const auto highEdge = juce::jmap(bandSpread, 600.0f, static_cast<float>(sampleRate) * 0.5f);
            const auto transition = 35.0f + bandSpread * 480.0f;
            const auto lowFade = clamp01((frequency - lowEdge) / transition);
            const auto highFade = clamp01((highEdge - frequency) / transition);
            auto bandWet = bandSpread > 0.985f ? 1.0f : std::pow(lowFade * highFade, 0.55f);

            const auto lfo = 0.5f + 0.5f * std::sin(travel + std::pow(binNorm, 0.52f) * pi * (3.0f + bandSpread * 24.0f));
            const auto flangeFrames = juce::jlimit(1, 54, 1 + static_cast<int>((1.0f + lfo * lfo * 53.0f) * (0.18f + depth * 0.82f)));
            const auto historyIndex = static_cast<size_t>(spectralHistoryIndex(flangeFrames, bin, state.spectralHistoryWritePosition));
            const auto delayedReal = state.spectralHistoryReal[historyIndex];
            const auto delayedImag = state.spectralHistoryImag[historyIndex];
            const auto flangePolarity = std::sin(travel * 0.37f + binNorm * pi * (9.0f + bandSpread * 38.0f));
            const auto flangeMix = bandWet * depth * (0.45f + (1.0f - morph) * 1.65f);
            const auto realIndex = static_cast<size_t>(bin * 2);
            const auto imagIndex = realIndex + 1;
            const auto real = state.fftData[realIndex];
            const auto imag = state.fftData[imagIndex];
            const auto flangedReal = real + delayedReal * flangePolarity * flangeMix * (0.35f + flangerAmount * 1.65f);
            const auto flangedImag = imag + delayedImag * flangePolarity * flangeMix * (0.35f + flangerAmount * 1.65f);
            const auto phaserNotch = 0.5f + 0.5f * std::sin(travel * (0.65f + motion * 3.4f) + binNorm * pi * (8.0f + bandSpread * 58.0f));
            const auto phaserGain = 1.0f + bandWet * depth * phaserAmount * ((phaserNotch - 0.5f) * 1.25f + 0.22f);
            const auto phaseOffset = bandWet * depth * phaserAmount * (std::sin(travel * 1.33f + binNorm * pi * 19.0f) * pi * (0.85f + binNorm * 2.25f));
            const auto cosine = std::cos(phaseOffset);
            const auto sine = std::sin(phaseOffset);
            const auto phasedReal = (real * cosine - imag * sine) * phaserGain;
            const auto phasedImag = (real * sine + imag * cosine) * phaserGain;
            const auto wetReal = real + (phasedReal - real) * phaserAmount + (flangedReal - real) * flangerAmount;
            const auto wetImag = imag + (phasedImag - imag) * phaserAmount + (flangedImag - imag) * flangerAmount;
            const auto morphCompensation = 1.0f + bandWet * std::max(phaserAmount, flangerAmount) * (0.25f + depth * 0.24f);
            state.fftData[realIndex] = juce::jmap(bandWet, real, wetReal * morphCompensation);
            state.fftData[imagIndex] = juce::jmap(bandWet, imag, wetImag * morphCompensation);
            processed = juce::jmap(bandWet, original, safeMagnitude(state.fftData[realIndex], state.fftData[imagIndex]));
        }
        else if (parameters.mode == Mode::rust)
        {
            const auto corrode = clamp01(parameters.drive);
            const auto fold = parameters.rot;
            const auto anchor = clamp01((parameters.subGuardHz - 20.0f) / 240.0f);
            const auto edge = 1.0f;
            const auto movement = std::sin(state.phase + static_cast<float>(bin) * 0.08f);
            const auto instability = 1.0f + movement * 0.18f * (0.25f + fold * 0.75f);
            const auto threshold = envelope * (0.98f - (fold * 0.8f));
            const auto excess = juce::jmax(0.0f, original - threshold);
            const auto excessRatio = excess / (envelope + 1.0e-5f);
            const auto rustAmount = std::tanh(excessRatio * (0.9f + std::pow(corrode, 0.45f) * 8.5f));
            const auto foldLift = 0.82f + fold * 0.82f;
            processed = original + envelope * rustAmount * edge * instability * foldLift;

            const auto reverseSuck = std::pow(1.0f - fold, 3.0f) * (1.0f - fold * 0.75f);
            if (reverseSuck > 0.001f && fold < 0.82f)
            {
                const auto smearFrames = juce::jlimit(1, maxSpectralDelayFrames - 1,
                    static_cast<int>(3.0f + reverseSuck * (10.0f + binNorm * 18.0f)));
                const auto historyIndex = static_cast<size_t>(spectralHistoryIndex(smearFrames, bin, state.spectralHistoryWritePosition));
                const auto delayedReal = state.spectralHistoryReal[historyIndex];
                const auto delayedImag = state.spectralHistoryImag[historyIndex];
                const auto delayedMag = safeMagnitude(delayedReal, delayedImag);
                const auto suck = reverseSuck * (0.45f + corrode * 0.4f) * (1.0f - anchor * 0.55f);
                const auto realIndex = static_cast<size_t>(bin * 2);
                const auto imagIndex = realIndex + 1;

                processed = juce::jmap(suck, processed, delayedMag * edge * (0.75f + corrode * 0.9f));
                state.fftData[realIndex] = juce::jmap(suck, state.fftData[realIndex], delayedReal);
                state.fftData[imagIndex] = juce::jmap(suck, state.fftData[imagIndex], delayedImag);
            }

            const auto foldAmount = std::pow(fold, 0.55f);
            const auto mirror = juce::jlimit(1, numBins - 2,
                static_cast<int>(std::round((1.0f - std::abs((binNorm * 2.0f) - 1.0f)) * static_cast<float>(numBins - 2))));
            const auto warped = juce::jlimit(1, numBins - 2,
                static_cast<int>(std::round(std::pow(binNorm, 0.55f + fold * 0.85f) * static_cast<float>(numBins - 2))));
            const auto scatter = juce::jlimit(1, numBins - 2,
                bin + static_cast<int>(std::round(std::sin(static_cast<float>(bin) * 0.29f + state.phase * 3.0f) * fold * 12.0f)));
            const auto spreadAmount = original * rustAmount * foldAmount * (0.34f + anchor * 0.26f);
            state.spectralAdditions[static_cast<size_t>(mirror)] += spreadAmount * 0.36f;
            state.spectralAdditions[static_cast<size_t>(juce::jlimit(1, numBins - 2, mirror - 2))] += spreadAmount * 0.16f;
            state.spectralAdditions[static_cast<size_t>(juce::jlimit(1, numBins - 2, mirror + 2))] += spreadAmount * 0.16f;
            state.spectralAdditions[static_cast<size_t>(warped)] += spreadAmount * 0.38f;
            state.spectralAdditions[static_cast<size_t>(scatter)] += spreadAmount * 0.30f;
            processed = juce::jmap(foldAmount * 0.72f, processed, processed * (0.74f + fold * 0.16f) + envelope * rustAmount * (0.42f + fold * 0.38f));
        }
        else if (parameters.mode == Mode::glass)
        {
            const auto delayAmount = parameters.drive;
            const auto feedbackAmount = parameters.rot;
            const auto texture = juce::jlimit(-1.0f, 1.0f, parameters.lock);
            const auto smear = std::max(0.0f, texture);
            const auto bubble = std::max(0.0f, -texture);
            const auto pitchDrift = std::abs(parameters.tilt);
            const auto shatter = clamp01((parameters.subGuardHz - 20.0f) / 240.0f);
            const auto reverse = std::pow(clamp01(parameters.glassReverse), 0.45f);
            const auto realIndex = static_cast<size_t>(bin * 2);
            const auto imagIndex = realIndex + 1;
            const auto real = state.fftData[realIndex];
            const auto imag = state.fftData[imagIndex];

            const auto delaySeconds = 0.035f + std::pow(juce::jlimit(0.0f, 1.0f, delayAmount), 1.55f) * 1.45f;
            const auto baseDelayFrames = juce::jlimit(1, maxSpectralDelayFrames - 1,
                static_cast<int>(std::round((delaySeconds * static_cast<float>(sampleRate)) / static_cast<float>(juce::jmax(1, hopSize)))));
            const auto stereoOffset = static_cast<int>(std::round(state.stereoSign * shatter * 5.0f));
            const auto delayFrames = juce::jlimit(1, maxSpectralDelayFrames - 1, baseDelayFrames + stereoOffset);

            const auto driftSeed = std::sin(std::floor(static_cast<float>(state.frameCounter) * 0.17f) * 1.913f + static_cast<float>(bin) * 0.071f + state.stereoSign * 2.3f);
            const auto driftTextureScale = 0.85f + bubble * 0.55f + smear * 0.35f;
            const auto randomSemitones = driftSeed * pitchDrift * 18.0f * driftTextureScale;
            const auto driftRatio = std::pow(2.0f, randomSemitones / 12.0f);
            const auto driftBin = juce::jlimit(1, numBins - 2, static_cast<int>(std::round(static_cast<float>(bin) / driftRatio)));
            const auto shatterScatter = std::sin(static_cast<float>(state.frameCounter) * 0.41f + static_cast<float>(bin) * 0.137f + state.stereoSign * 2.1f);
            const auto textureScatter = shatter * (smear * 6.0f + bubble * 62.0f);
            const auto shatterBin = juce::jlimit(1, numBins - 2, bin + static_cast<int>(std::round(shatterScatter * textureScatter)));

            const auto cleanIndex = static_cast<size_t>(glassDelayIndex(delayFrames, bin, state.spectralHistoryWritePosition));
            const auto reverseWindow = juce::jlimit(8, maxSpectralDelayFrames - 1,
                static_cast<int>(14.0f + std::pow(delayAmount, 0.68f) * static_cast<float>(maxSpectralDelayFrames - 15)));
            const auto reverseCycle = static_cast<int>((state.frameCounter * (1 + static_cast<int>(delayAmount * 2.0f)))
                % static_cast<unsigned int>(juce::jmax(2, reverseWindow)));
            const auto reverseSweep = juce::jlimit(1, maxSpectralDelayFrames - 1,
                juce::jmax(1, reverseWindow - reverseCycle));
            const auto textureIndex = static_cast<size_t>(glassDelayIndex(delayFrames, shatterBin, state.spectralHistoryWritePosition));
            const auto driftIndex = static_cast<size_t>(glassDelayIndex(delayFrames, driftBin, state.spectralHistoryWritePosition));
            const auto reverseIndex = static_cast<size_t>(glassDelayIndex(reverseSweep, driftBin, state.spectralHistoryWritePosition));
            const auto cleanReal = state.glassFeedbackReal[cleanIndex];
            const auto cleanImag = state.glassFeedbackImag[cleanIndex];
            const auto textureReal = state.glassFeedbackReal[textureIndex];
            const auto textureImag = state.glassFeedbackImag[textureIndex];
            const auto driftReal = state.glassFeedbackReal[driftIndex];
            const auto driftImag = state.glassFeedbackImag[driftIndex];
            const auto reverseReal = state.glassFeedbackReal[reverseIndex];
            const auto reverseImag = state.glassFeedbackImag[reverseIndex];
            const auto smearFrames = juce::jlimit(1, maxSpectralDelayFrames - 1, delayFrames + static_cast<int>(bubble * (12.0f + delayAmount * 58.0f)));
            const auto bubbleIndex = static_cast<size_t>(glassDelayIndex(smearFrames, shatterBin, state.spectralHistoryWritePosition));
            const auto bubbleHistoryReal = state.glassFeedbackReal[bubbleIndex];
            const auto bubbleHistoryImag = state.glassFeedbackImag[bubbleIndex];
            const auto sharpen = 1.0f + smear * (0.65f + binNorm * 0.55f);
            const auto smearReal = (cleanReal * (1.0f - smear * 0.18f) + textureReal * smear * 0.18f) * sharpen;
            const auto smearImag = (cleanImag * (1.0f - smear * 0.18f) + textureImag * smear * 0.18f) * sharpen;
            const auto bubbleReal = (cleanReal * (1.0f - bubble * 0.82f) + (textureReal * 0.24f + bubbleHistoryReal * 0.76f) * bubble * 0.82f) * (1.0f - bubble * 0.18f);
            const auto bubbleImag = (cleanImag * (1.0f - bubble * 0.82f) + (textureImag * 0.24f + bubbleHistoryImag * 0.76f) * bubble * 0.82f) * (1.0f - bubble * 0.18f);
            const auto spectralReal = texture >= 0.0f ? smearReal : bubbleReal;
            const auto spectralImag = texture >= 0.0f ? smearImag : bubbleImag;
            const auto driftMix = pitchDrift * (0.18f + bubble * 0.24f + smear * 0.12f);
            const auto driftedReal = juce::jmap(driftMix, spectralReal, driftReal);
            const auto driftedImag = juce::jmap(driftMix, spectralImag, driftImag);
            const auto shatterMix = shatter * (0.08f + bubble * 0.86f + smear * 0.18f);
            const auto shatteredReal = juce::jmap(shatterMix, driftedReal, textureReal);
            const auto shatteredImag = juce::jmap(shatterMix, driftedImag, textureImag);
            const auto swaySoftener = 1.0f - reverse * (0.06f + (1.0f - delayAmount) * 0.18f);
            const auto delayReal = juce::jmap(reverse, shatteredReal, reverseReal * swaySoftener);
            const auto delayImag = juce::jmap(reverse, shatteredImag, reverseImag * swaySoftener);
            const auto delayMag = safeMagnitude(delayReal, delayImag);
            const auto repeatLevel = 0.72f + feedbackAmount * 0.18f;
            const auto writeIndex = static_cast<size_t>(state.spectralHistoryWritePosition * numBins + bin);
            const auto inputSend = 0.82f;

            processed = original + delayMag * repeatLevel * (0.95f + bubble * 0.26f + smear * 0.34f);
            state.fftData[realIndex] = real + delayReal * repeatLevel;
            state.fftData[imagIndex] = imag + delayImag * repeatLevel;

            state.glassFeedbackReal[writeIndex] = real * inputSend;
            state.glassFeedbackImag[writeIndex] = imag * inputSend;
        }

        state.processedMagnitudes[static_cast<size_t>(bin)] = processed;
    }

    const auto lockAmount = modeLockAmount();

    for (int bin = 1; bin < numBins - 1; ++bin)
    {
        const auto frequency = (static_cast<float>(bin) * static_cast<float>(sampleRate)) / static_cast<float>(fftSize);
        const auto protection = protectionForFrequency(frequency);
        const auto original = state.originalMagnitudes[static_cast<size_t>(bin)];
        const auto added = state.spectralAdditions[static_cast<size_t>(bin)];
        const auto binNorm = static_cast<float>(bin) / static_cast<float>(numBins - 1);
        const auto finalColor = (parameters.mode == Mode::melt || parameters.mode == Mode::rust)
            ? 1.0f
            : juce::Decibels::decibelsToGain(tilt * (binNorm - 0.36f) * 28.0f);
        auto finalDrive = 0.58f + parameters.drive * 0.82f;

        if (parameters.mode == Mode::melt)
            finalDrive = 1.0f + std::abs(parameters.drive) * 0.28f;
        else if (parameters.mode == Mode::rust)
            finalDrive = 0.62f + clamp01(parameters.drive) * 0.48f;
        else if (parameters.mode == Mode::glass)
            finalDrive = 0.86f;

        const auto driven = (state.processedMagnitudes[static_cast<size_t>(bin)] + added) * finalDrive * finalColor;
        const auto locked = original + ((driven - original) * (1.0f - lockAmount));
        const auto maxMagnitude = parameters.mode == Mode::rust ? (original * 9.0f + 2.0f) : (original * 14.0f + 4.0f);
        state.processedMagnitudes[static_cast<size_t>(bin)] = juce::jlimit(0.0f, maxMagnitude, juce::jmap(protection, locked, original));
    }

    for (int bin = 1; bin < numBins - 1; ++bin)
    {
        const auto realIndex = static_cast<size_t>(bin * 2);
        const auto imagIndex = realIndex + 1;
        const auto real = state.fftData[realIndex];
        const auto imag = state.fftData[imagIndex];
        const auto currentMagnitude = safeMagnitude(real, imag);
        const auto gain = state.processedMagnitudes[static_cast<size_t>(bin)] / currentMagnitude;

        state.fftData[realIndex] = real * gain;
        state.fftData[imagIndex] = imag * gain;
    }

    fft->performRealOnlyInverseTransform(state.fftData.data());
    window->multiplyWithWindowingTable(state.fftData.data(), static_cast<size_t>(fftSize));

    const auto compensation = modeOutputTrim();

    for (int i = 0; i < fftSize; ++i)
    {
        const auto ringIndex = (state.writePosition + i) % fftSize;
        state.outputRing[static_cast<size_t>(ringIndex)] += state.fftData[static_cast<size_t>(i)] * olaCompensation * compensation;
    }

    state.spectralHistoryWritePosition = (state.spectralHistoryWritePosition + 1) % maxSpectralDelayFrames;
}

float SpectralRotEngine::calculateLocalAverage(const std::vector<float>& magnitudes, int bin) const
{
    const auto radius = 5;
    auto total = 0.0f;
    auto count = 0.0f;

    for (int offset = -radius; offset <= radius; ++offset)
    {
        const auto index = juce::jlimit(0, numBins - 1, bin + offset);
        total += magnitudes[static_cast<size_t>(index)];
        count += 1.0f;
    }

    return total / count;
}

float SpectralRotEngine::protectionForFrequency(float frequencyHz) const
{
    const auto guard = juce::jlimit(20.0f, 260.0f, parameters.lowHoldHz);
    const auto width = juce::jmax(20.0f, guard * 0.5f);

    if (frequencyHz <= guard)
        return 1.0f;

    if (frequencyHz >= guard + width)
        return 0.0f;

    return 1.0f - clamp01((frequencyHz - guard) / width);
}

float SpectralRotEngine::modeLockAmount() const
{
    switch (parameters.mode)
    {
        case Mode::melt: return 0.0f;
        case Mode::rust: return 0.0f;
        case Mode::glass: return 0.0f;
    }

    return parameters.lock * 0.35f;
}

float SpectralRotEngine::modeTargetRmsMultiplier() const
{
    const auto driveLift = 1.0f + parameters.drive * 0.5f;

    switch (parameters.mode)
    {
        case Mode::melt: return driveLift * 0.98f;
        case Mode::rust: return driveLift * 0.95f;
        case Mode::glass: return driveLift * 0.78f;
    }

    return driveLift;
}

float SpectralRotEngine::modeOutputTrim() const
{
    switch (parameters.mode)
    {
        case Mode::melt: return 0.78f;
        case Mode::rust: return 0.78f;
        case Mode::glass: return 0.74f;
    }

    return 1.0f;
}

float SpectralRotEngine::nextNoise(ChannelState& state)
{
    state.noiseState = (1664525u * state.noiseState) + 1013904223u;
    return static_cast<float>((state.noiseState >> 8) & 0x00ffffffu) / static_cast<float>(0x01000000u);
}
