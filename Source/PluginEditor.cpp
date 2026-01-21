#include "PluginEditor.h"

PythonHookerAudioProcessorEditor::PythonHookerAudioProcessorEditor (PythonHookerAudioProcessor& p)
: AudioProcessorEditor(&p), processor(p)
{
    title.setText("PythonHooker (MIDI Proxy)", juce::dontSendNotification);
    title.setJustificationType(juce::Justification::centred);
    title.setFont(juce::Font(18.0f, juce::Font::bold));

    configPath.setText("Config: " + processor.getConfigDirPath(), juce::dontSendNotification);
    configPath.setJustificationType(juce::Justification::centredLeft);

    // Very simple status: config load success vs error/warnings
    // (We don't expose the whole error string in POC UI; you can expand later.)
    status.setText("POC: loads event.conf once on startup.", juce::dontSendNotification);
    status.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(title);
    addAndMakeVisible(configPath);
    addAndMakeVisible(status);

    setSize(520, 140);
}

void PythonHookerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
}

void PythonHookerAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced(12);

    title.setBounds(r.removeFromTop(30));
    r.removeFromTop(8);

    configPath.setBounds(r.removeFromTop(24));
    status.setBounds(r.removeFromTop(24));
}
