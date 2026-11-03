#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class PulseForgeAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit PulseForgeAudioProcessorEditor(PulseForgeAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    PulseForgeAudioProcessor& processor;
    juce::Label title, subtitle, mutationLabel, gainLabel;
    juce::Slider mutation, gain;
    std::array<juce::Label, 4> rowLabels, sourceLabels, sampleLabels;
    std::array<std::unique_ptr<juce::TextButton>, 4> sourceButtons, loadButtons;
    std::array<std::array<std::unique_ptr<juce::TextButton>, 16>, 4> steps;
    juce::FileChooser chooser { "Load drum sample", juce::File::getSpecialLocation(juce::File::userHomeDirectory), "*.wav;*.aif;*.aiff" };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PulseForgeAudioProcessorEditor)
};
