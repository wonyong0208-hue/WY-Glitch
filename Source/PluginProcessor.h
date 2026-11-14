#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>
#include <random>

class WYGlitchAudioProcessor : public juce::AudioProcessor
{
public:
    WYGlitchAudioProcessor();
    ~WYGlitchAudioProcessor() override = default;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "WY Glitch"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorValueTreeState parameters;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
private:
    static constexpr int bufferSize = 131072;
    std::array<std::vector<float>, 2> history;
    int writePos = 0;
    int stepPos = 0;
    int stepLength = 2048;
    int sliceStart = 0;
    int sliceLength = 2048;
    int direction = 1;
    int holdCounter = 0;
    std::array<float,2> held {0.0f,0.0f};
    std::array<float,2> lowpass {0.0f,0.0f};
    double sampleRateHz = 44100.0;
    std::mt19937 random {0x5747594Cu};
    float randomUnit();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WYGlitchAudioProcessor)
};
