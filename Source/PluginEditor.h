#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

class PythonHookerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit PythonHookerAudioProcessorEditor (PythonHookerAudioProcessor&);
    ~PythonHookerAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    PythonHookerAudioProcessor& processor;

    juce::Label title;
    juce::Label configPath;
    juce::Label status;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PythonHookerAudioProcessorEditor)
};
