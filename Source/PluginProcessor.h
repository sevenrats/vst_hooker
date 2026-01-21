#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <atomic>
#include "ConfigLoader.h"

// Job enqueued from audio thread -> executed on worker thread
struct ScriptJob
{
    juce::File script;      // absolute path to .py
    int note = 0;
    int velocity = 0;
    int channel = 1;        // 1-16
    double hostTimeSeconds = 0.0;
};

class PythonHookerAudioProcessor : public juce::AudioProcessor,
                                  private juce::Thread
{
public:
    PythonHookerAudioProcessor();
    ~PythonHookerAudioProcessor() override;

    // AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // For editor debug display
    juce::String getConfigDirPath() const;

private:
    // Thread
    void run() override;

    // Queue (lock-free) from audio thread to worker thread
    static constexpr int kQueueSize = 1024;
    std::array<ScriptJob, kQueueSize> jobQueue;
    juce::AbstractFifo fifo { kQueueSize };

    std::atomic<bool> running { false };
    std::atomic<double> sr { 44100.0 };

    // Config
    LoadedConfig loadedConfig;
    juce::String pythonExecutable = "python3"; // POC: rely on PATH

    // Execution
    void executeJob(const ScriptJob& job);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PythonHookerAudioProcessor)
};
