#include "PluginProcessor.h"
#include "PluginEditor.h"

PythonHookerAudioProcessor::PythonHookerAudioProcessor()
: juce::AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::disabled(), true)
                        .withOutput ("Output", juce::AudioChannelSet::disabled(), true)),
  juce::Thread ("PythonHookWorker")
{
}

PythonHookerAudioProcessor::~PythonHookerAudioProcessor()
{
    running.store(false);
    stopThread(1500);
}

const juce::String PythonHookerAudioProcessor::getName() const
{
    return "PythonHooker";
}

juce::String PythonHookerAudioProcessor::getConfigDirPath() const
{
    return loadedConfig.configDir.getFullPathName();
}

bool PythonHookerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // MIDI effect: no audio I/O
    return layouts.getMainInputChannelSet()  == juce::AudioChannelSet::disabled()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled();
}

void PythonHookerAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    sr.store(sampleRate);

    // Load config once (POC)
    loadedConfig = ConfigLoader::loadDefaultConfig();

    // Start worker thread
    running.store(true);
    startThread(juce::Thread::Priority::normal);
}

void PythonHookerAudioProcessor::releaseResources()
{
    running.store(false);
    stopThread(1500);
}

void PythonHookerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear(); // no audio

    // Get approximate host time for informational purposes.
    double hostTimeSeconds = 0.0;
    if (auto* playHead = getPlayHead())
    {
        juce::AudioPlayHead::CurrentPositionInfo pos;
        if (playHead->getCurrentPosition(pos))
            hostTimeSeconds = pos.timeInSeconds;
    }

    // Forward MIDI unchanged. While iterating, enqueue jobs for matched NoteOn.
    juce::MidiBuffer out;
    out.ensureSize(midiMessages.getNumEvents());

    for (const auto meta : midiMessages)
    {
        const auto& msg = meta.getMessage();
        const int samplePos = meta.samplePosition;

        // Always forward unchanged event with original sample position
        out.addEvent(msg, samplePos);

        // POC: react only to note-on with velocity > 0
        if (msg.isNoteOn())
        {
            const int note = msg.getNoteNumber();
            const int vel  = (int) msg.getVelocity(); // 0..127
            const int chan = msg.getChannel();        // 1..16

            auto it = loadedConfig.map.find(note);
            if (it != loadedConfig.map.end())
            {
                ScriptJob job;
                job.script = it->second;
                job.note = note;
                job.velocity = vel;
                job.channel = chan;

                // Approximate absolute time of this event
                job.hostTimeSeconds = hostTimeSeconds + (double(samplePos) / sr.load());

                // Enqueue (never block on audio thread)
                int start1, size1, start2, size2;
                fifo.prepareToWrite(1, start1, size1, start2, size2);

                if (size1 > 0)
                {
                    jobQueue[(size_t) start1] = job;
                    fifo.finishedWrite(1);
                }
                else
                {
                    // Queue full: drop the hook (MIDI still forwarded)
                    // Do NOT log here (logging can block)
                }
            }
        }
    }

    midiMessages.swapWith(out);
}

void PythonHookerAudioProcessor::run()
{
    while (! threadShouldExit() && running.load())
    {
        int start1, size1, start2, size2;

        // Read up to 32 jobs per iteration
        fifo.prepareToRead(32, start1, size1, start2, size2);

        if (size1 == 0 && size2 == 0)
        {
            // Avoid busy looping
            wait(2);
            continue;
        }

        for (int i = 0; i < size1; ++i)
            executeJob(jobQueue[(size_t) (start1 + i)]);

        for (int i = 0; i < size2; ++i)
            executeJob(jobQueue[(size_t) (start2 + i)]);

        fifo.finishedRead(size1 + size2);
    }
}

void PythonHookerAudioProcessor::executeJob(const ScriptJob& job)
{
    // Basic safety: ensure script still exists
    if (! job.script.existsAsFile())
        return;

    // Spawn python:
    // python3 <script> --note N --vel V --chan C --time T
    juce::StringArray args;
    args.add(pythonExecutable);
    args.add(job.script.getFullPathName());
    args.add("--note"); args.add(juce::String(job.note));
    args.add("--vel");  args.add(juce::String(job.velocity));
    args.add("--chan"); args.add(juce::String(job.channel));
    args.add("--time"); args.add(juce::String(job.hostTimeSeconds, 6));

    juce::ChildProcess proc;

    // Start and wait briefly (POC behavior).
    // If you want "fire and forget", you can omit waiting.
    const bool started = proc.start(args);

    if (started)
    {
        // POC: allow up to 2 seconds; scripts should be fast
        proc.waitForProcessToFinish(2000);
    }
}

bool PythonHookerAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* PythonHookerAudioProcessor::createEditor()
{
    return new PythonHookerAudioProcessorEditor(*this);
}

void PythonHookerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Minimal POC state: store python executable string (so later you can override it in UI)
    juce::MemoryOutputStream stream(destData, true);
    stream.writeString(pythonExecutable);
}

void PythonHookerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::MemoryInputStream stream(data, (size_t) sizeInBytes, false);
    pythonExecutable = stream.readString();
}

// JUCE factory
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PythonHookerAudioProcessor();
}
