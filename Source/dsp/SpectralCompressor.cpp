#include "dsp/SpectralCompressor.h"

namespace
{
    constexpr float pi = juce::MathConstants<float>::pi;

    float clamp01(float value)
    {
        return juce::jlimit(0.0f, 1.0f, value);
    }

    float timeToCoefficient(float milliseconds, double sampleRate)
    {
        const auto samples = juce::jmax(1.0f, milliseconds * 0.001f * static_cast<float>(sampleRate));
        return 1.0f - std::exp(-1.0f / samples);
    }

    float softKnee(float value, float knee)
    {
        if (value <= 0.0f)
            return 0.0f;

        return (value * value) / (value + knee);
    }
}

void SpectralCompressor::prepare(double newSampleRate, int channels)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    states.resize(static_cast<size_t>(juce::jmax(1, channels)));
    updateCoefficients();
    reset();
}

void SpectralCompressor::reset()
{
    for (auto& state : states)
    {
        state.lowpass.fill(0.0f);
        state.envelope.fill(0.0f);
        state.average.fill(1.0e-4f);
        state.gain.fill(1.0f);
        state.activityLevel = 1.0e-4f;
        state.transientProtectSamples = 0;
    }
}

void SpectralCompressor::setParameters(const Parameters& newParameters)
{
    parameters = newParameters;
}

void SpectralCompressor::updateCoefficients()
{
    const float cutoffs[bandCount - 1] = {
        120.0f, 230.0f, 420.0f, 760.0f, 1350.0f, 2400.0f, 4300.0f, 7600.0f, 12500.0f
    };

    for (int i = 0; i < bandCount - 1; ++i)
    {
        const auto cutoff = juce::jlimit(20.0f, static_cast<float>(sampleRate * 0.46), cutoffs[i]);
        coefficients[static_cast<size_t>(i)] = 1.0f - std::exp((-2.0f * pi * cutoff) / static_cast<float>(sampleRate));
    }

    for (int i = 0; i < bandCount; ++i)
    {
        const auto norm = static_cast<float>(i) / static_cast<float>(bandCount - 1);
        focusWeights[static_cast<size_t>(i)] = 0.42f + std::pow(norm, 0.75f) * 0.92f;
    }
}

void SpectralCompressor::process(juce::AudioBuffer<float>& buffer)
{
    if (! parameters.enabled || parameters.tame <= 0.001f || states.empty())
    {
        lastGainReduction = 0.0f;
        return;
    }

    const auto channels = juce::jmin(buffer.getNumChannels(), static_cast<int>(states.size()));
    const auto samples = buffer.getNumSamples();
    const auto tame = clamp01(parameters.tame);
    const auto focus = clamp01(parameters.focus);
    const auto speed = clamp01(parameters.speed);
    const auto thresholdLinear = juce::Decibels::decibelsToGain(parameters.thresholdDb);
    const auto speedCurve = std::pow(speed, 0.72f);
    const auto detectorAttack = timeToCoefficient(16.0f - speedCurve * 13.0f, sampleRate);
    const auto detectorRelease = timeToCoefficient(1.0f, sampleRate);
    const auto gainAttack = timeToCoefficient(70.0f - speedCurve * 64.0f, sampleRate);
    const auto gainRelease = timeToCoefficient(1.0f, sampleRate);
    const auto averageRate = timeToCoefficient(650.0f - speedCurve * 420.0f, sampleRate);
    const auto activityAttack = timeToCoefficient(4.0f, sampleRate);
    const auto activityRelease = timeToCoefficient(180.0f, sampleRate);
    const auto protectLength = static_cast<int>((34.0f - speedCurve * 20.0f) * 0.001f * static_cast<float>(sampleRate));
    const auto crushZone = std::pow(juce::jmax(0.0f, (tame - 0.72f) / 0.28f), 2.15f);
    const auto ratio = 1.35f + std::pow(tame, 0.72f) * (5.8f + focus * 5.2f) + crushZone * (12.0f + focus * 6.5f);
    auto reductionSum = 0.0f;
    auto reductionSamples = 0;

    for (int channel = 0; channel < channels; ++channel)
    {
        auto* data = buffer.getWritePointer(channel);
        auto& state = states[static_cast<size_t>(channel)];

        for (int sample = 0; sample < samples; ++sample)
        {
            const auto input = data[sample];
            std::array<float, bandCount - 1> lowpass {};

            for (int i = 0; i < bandCount - 1; ++i)
            {
                auto& lp = state.lowpass[static_cast<size_t>(i)];
                lp += coefficients[static_cast<size_t>(i)] * (input - lp);
                lowpass[static_cast<size_t>(i)] = lp;
            }

            std::array<float, bandCount> bands {};
            bands[0] = lowpass[0];

            for (int i = 1; i < bandCount - 1; ++i)
                bands[static_cast<size_t>(i)] = lowpass[static_cast<size_t>(i)] - lowpass[static_cast<size_t>(i - 1)];

            bands[bandCount - 1] = input - lowpass[bandCount - 2];
            auto broadbandLevel = 0.0f;

            for (int band = 0; band < bandCount; ++band)
            {
                const auto bandValue = bands[static_cast<size_t>(band)];
                const auto level = std::abs(bandValue) + 1.0e-7f;
                auto& env = state.envelope[static_cast<size_t>(band)];
                broadbandLevel += level;

                env += (level - env) * (level > env ? detectorAttack : detectorRelease);
            }

            const auto previousActivity = state.activityLevel;
            state.activityLevel += (broadbandLevel - state.activityLevel) * (broadbandLevel > state.activityLevel ? activityAttack : activityRelease);

            if (previousActivity < 0.0012f && broadbandLevel > previousActivity * 7.5f + 0.0018f)
            {
                state.transientProtectSamples = juce::jmax(state.transientProtectSamples, protectLength);
                state.gain.fill(1.0f);

                for (int band = 0; band < bandCount; ++band)
                    state.average[static_cast<size_t>(band)] = juce::jmax(state.average[static_cast<size_t>(band)],
                                                                          state.envelope[static_cast<size_t>(band)] * 1.15f);
            }

            const auto transientProtect = state.transientProtectSamples > 0
                ? static_cast<float>(state.transientProtectSamples) / static_cast<float>(juce::jmax(1, protectLength))
                : 0.0f;

            for (int band = 0; band < bandCount; ++band)
            {
                const auto env = state.envelope[static_cast<size_t>(band)];
                auto& avg = state.average[static_cast<size_t>(band)];
                avg = juce::jmax(avg, env * (0.18f + transientProtect * 0.96f));
                avg += (env - avg) * juce::jmap(transientProtect, averageRate, timeToCoefficient(24.0f, sampleRate));
            }

            auto output = 0.0f;
            std::array<float, bandCount> targetGains {};

            for (int band = 0; band < bandCount; ++band)
            {
                const auto env = state.envelope[static_cast<size_t>(band)];
                const auto avg = state.average[static_cast<size_t>(band)];
                auto neighbourAverage = avg;

                if (band > 0)
                    neighbourAverage += state.envelope[static_cast<size_t>(band - 1)];

                if (band < bandCount - 1)
                    neighbourAverage += state.envelope[static_cast<size_t>(band + 1)];

                neighbourAverage /= band == 0 || band == bandCount - 1 ? 2.0f : 3.0f;

                const auto thresholdPeak = env / (thresholdLinear + 1.0e-7f);
                const auto peakVsNeighbours = env / (neighbourAverage + thresholdLinear * 0.08f + 1.0e-7f);
                const auto broadOvershoot = juce::jmax(0.0f, thresholdPeak - 1.0f);
                const auto narrowOvershoot = juce::jmax(0.0f, peakVsNeighbours - (1.0f + thresholdLinear * 0.22f));
                auto overshoot = juce::jmap(focus, broadOvershoot, narrowOvershoot * focusWeights[static_cast<size_t>(band)] * 1.45f);
                overshoot = softKnee(overshoot, 0.32f - focus * 0.14f);

                auto targetGain = 1.0f / (1.0f + overshoot * ratio * tame);

                if (transientProtect > 0.0f)
                    targetGain = juce::jmap(std::sqrt(transientProtect), targetGain, juce::jmax(targetGain, 0.965f - crushZone * 0.08f));

                targetGains[static_cast<size_t>(band)] = targetGain;
            }

            for (int band = 0; band < bandCount; ++band)
            {
                auto targetGain = targetGains[static_cast<size_t>(band)];
                auto weight = 1.0f;

                if (band > 0)
                {
                    targetGain += targetGains[static_cast<size_t>(band - 1)] * 0.18f;
                    weight += 0.18f;
                }

                if (band < bandCount - 1)
                {
                    targetGain += targetGains[static_cast<size_t>(band + 1)] * 0.18f;
                    weight += 0.18f;
                }

                targetGain /= weight;
                targetGain = juce::jlimit(0.035f + (1.0f - crushZone) * 0.065f, 1.0f, targetGain);

                auto& smoothedGain = state.gain[static_cast<size_t>(band)];
                smoothedGain += (targetGain - smoothedGain) * (targetGain < smoothedGain ? gainAttack : gainRelease);
                reductionSum += 1.0f - smoothedGain;
                ++reductionSamples;

                const auto bandNorm = static_cast<float>(band) / static_cast<float>(bandCount - 1);
                const auto warmTiltDb = (0.42f - bandNorm) * (1.0f - focus) * 1.8f;
                const auto clarityTiltDb = (bandNorm - 0.36f) * focus * 3.8f;
                const auto toneGain = juce::Decibels::decibelsToGain(warmTiltDb + clarityTiltDb);
                output += bands[static_cast<size_t>(band)] * smoothedGain * toneGain;
            }

            auto compressed = juce::jmap(juce::jlimit(0.0f, 1.0f, tame * (0.98f + crushZone * 0.02f)), input, output);
            const auto lowBody = bands[0] + bands[1] + bands[2] * 0.45f;
            const auto transient = input - lowpass[static_cast<size_t>(juce::jmin(3, bandCount - 2))];
            const auto bloom = lowBody * tame * (1.0f - focus) * 0.085f;
            const auto smack = transient * tame * (0.035f + speedCurve * 0.17f) * (0.55f + focus * 0.45f);
            const auto characterDrive = 1.0f + tame * (0.85f + crushZone * 0.95f);
            const auto biased = (compressed + bloom + smack) * characterDrive
                              + (compressed * compressed) * tame * 0.045f;
            compressed = std::tanh(biased) / std::tanh(characterDrive);

            data[sample] = compressed;

            if (state.transientProtectSamples > 0)
                --state.transientProtectSamples;
        }
    }

    lastGainReduction = reductionSamples > 0
        ? juce::jlimit(0.0f, 1.0f, reductionSum / static_cast<float>(reductionSamples))
        : 0.0f;
}
