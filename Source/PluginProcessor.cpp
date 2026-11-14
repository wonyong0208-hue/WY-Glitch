#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

WYGlitchAudioProcessor::WYGlitchAudioProcessor()
 : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                   .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
   parameters(*this, nullptr, "PARAMETERS", createLayout()) {}

juce::AudioProcessorValueTreeState::ParameterLayout WYGlitchAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"mode", 1}, "Texture", juce::StringArray{"LIQUID", "CRYSTAL", "ORGANIC", "PLASMA"}, 1));
    for (const auto* name : {"glitch", "speed", "chaos", "mix"})
    {
        const float initial = juce::String(name) == "glitch" ? 55.0f : juce::String(name) == "speed" ? 65.0f : juce::String(name) == "chaos" ? 40.0f : 80.0f;
        p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{name, 1}, juce::String(name).toUpperCase(), juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), initial));
    }
    return p;
}

bool WYGlitchAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    return l.getMainInputChannelSet() == l.getMainOutputChannelSet() &&
           (l.getMainInputChannelSet() == juce::AudioChannelSet::mono() || l.getMainInputChannelSet() == juce::AudioChannelSet::stereo());
}

void WYGlitchAudioProcessor::prepareToPlay(double sr, int)
{
    sampleRateHz = sr;
    for (auto& channel : history) channel.assign(bufferSize, 0.0f);
    writePos = stepPos = sliceStart = holdCounter = 0;
    sliceLength = stepLength = 2048;
    held.fill(0.0f);
    lowpass.fill(0.0f);
}

float WYGlitchAudioProcessor::randomUnit()
{
    return static_cast<float>(random()) / static_cast<float>(random.max());
}

void WYGlitchAudioProcessor::processBlock(juce::AudioBuffer<float>& audio, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = juce::jmin(audio.getNumChannels(), 2);
    const int samples = audio.getNumSamples();
    const int mode = juce::jlimit(0, 3, static_cast<int>(parameters.getRawParameterValue("mode")->load()));
    const float amount = parameters.getRawParameterValue("glitch")->load() / 100.0f;
    const float speed = parameters.getRawParameterValue("speed")->load() / 100.0f;
    const float chaos = parameters.getRawParameterValue("chaos")->load() / 100.0f;
    const float mix = parameters.getRawParameterValue("mix")->load() / 100.0f;
    double bpm = 120.0;
    if (auto* playHead = getPlayHead())
        if (auto position = playHead->getPosition())
            if (auto hostBpm = position->getBpm())
                if (*hostBpm > 20.0 && *hostBpm < 400.0) bpm = *hostBpm;
    // Tempo-synced note values: 1/4, 1/8, 1/16, 1/32, 1/64.
    const int division = juce::jlimit(0, 4, static_cast<int>(speed * 4.999f));
    const int newLength = juce::jlimit(64, bufferSize / 4, static_cast<int>(sampleRateHz * 60.0 / bpm / (1 << division)));
    stepLength = newLength;
    for (int i = 0; i < samples; ++i)
    {
        if (stepPos >= stepLength)
        {
            stepPos = 0;
            sliceLength = stepLength;
            const bool trigger = randomUnit() < amount * (0.25f + chaos * 0.75f);
            direction = trigger && randomUnit() < chaos * (mode == 1 ? 0.8f : 0.45f) ? -1 : 1;
            // A triggered step replays earlier captured audio rather than silently cutting it.
            sliceStart = (writePos - sliceLength * (trigger ? 1 + static_cast<int>(randomUnit() * (1 + chaos * 3)) : 1) + bufferSize * 8) % bufferSize;
            if (!trigger) sliceStart = (writePos - sliceLength + bufferSize) % bufferSize;
        }
        const float phase = static_cast<float>(stepPos) / static_cast<float>(juce::jmax(1, sliceLength));
        const float edge = juce::jmin(1.0f, juce::jmin(phase, 1.0f - phase) * 80.0f);
        const int offset = direction > 0 ? stepPos : sliceLength - 1 - stepPos;
        const int readPos = (sliceStart + offset + bufferSize) % bufferSize;
        for (int c = 0; c < channels; ++c)
        {
            const float dry = audio.getSample(c, i);
            history[static_cast<size_t>(c)][static_cast<size_t>(writePos)] = dry;
            float wet = history[static_cast<size_t>(c)][static_cast<size_t>(readPos)];
            if (mode == 0) // LIQUID: fluid pitch-like vibrato, filtered saturation.
            {
                const int wobble = static_cast<int>(std::sin((stepPos / sampleRateHz) * (3.0 + speed * 17.0) * juce::MathConstants<double>::twoPi) * (2.0 + chaos * 60.0));
                wet = history[static_cast<size_t>(c)][static_cast<size_t>((readPos + wobble + bufferSize) % bufferSize)];
                lowpass[static_cast<size_t>(c)] += (0.12f + 0.6f * (1.0f - amount)) * (wet - lowpass[static_cast<size_t>(c)]);
                wet = std::tanh(lowpass[static_cast<size_t>(c)] * (1.0f + amount * 2.5f));
            }
            else if (mode == 1) // CRYSTAL: shimmering, ring-modulated fragments.
            {
                const float hz = 300.0f + 2800.0f * chaos;
                const float carrier = std::sin(static_cast<float>(juce::MathConstants<double>::twoPi * hz * (stepPos / sampleRateHz)));
                wet = std::tanh(wet * (1.0f + amount * 3.0f)) * (1.0f - amount * 0.7f + carrier * amount * 0.7f);
            }
            else if (mode == 2) // ORGANIC: irregular sample-and-hold with warm saturation.
            {
                const int interval = 1 + static_cast<int>(chaos * amount * 50.0f);
                if (holdCounter % interval == 0) held[static_cast<size_t>(c)] = wet;
                wet = std::tanh(held[static_cast<size_t>(c)] * (1.0f + amount * 5.0f));
            }
            else // PLASMA: aggressive digital folding and bit reduction.
            {
                const float driven = wet * (1.0f + amount * 13.0f);
                const float folded = std::asin(std::sin(driven)) * (2.0f / juce::MathConstants<float>::pi);
                const float levels = static_cast<float>(1 << juce::jlimit(3, 12, 12 - static_cast<int>(amount * chaos * 9.0f)));
                wet = std::round(folded * levels) / levels;
            }
            const float processed = dry * (1.0f - amount) + wet * amount * edge;
            audio.setSample(c, i, dry * (1.0f - mix) + processed * mix);
        }
        ++writePos;
        if (writePos == bufferSize) writePos = 0;
        ++stepPos;
        ++holdCounter;
        if (holdCounter > 1000000) holdCounter = 0;
    }
}

void WYGlitchAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml()) copyXmlToBinary(*xml, dest);
}
void WYGlitchAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessorEditor* WYGlitchAudioProcessor::createEditor() { return new WYGlitchAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new WYGlitchAudioProcessor(); }
