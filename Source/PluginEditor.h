#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>

class WYGlitchAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit WYGlitchAudioProcessorEditor(WYGlitchAudioProcessor&);
    ~WYGlitchAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    struct Style : juce::LookAndFeel_V4
    {
        void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    } style;
    WYGlitchAudioProcessor& processor;
    std::array<juce::TextButton,4> modes;
    std::array<juce::Slider,4> knobs;
    std::array<juce::Label,4> captions;
    std::array<juce::Label,4> values;
    juce::ComboBox presets;
    juce::Label syncLabel;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::array<std::unique_ptr<SliderAttachment>,4> attachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    juce::ComboBox modeSelector;
    float phase = 0.0f;
    void updateModes();
    void timerCallback() override;
    void loadPreset(int);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WYGlitchAudioProcessorEditor)
};
