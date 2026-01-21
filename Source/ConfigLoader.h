#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <unordered_map>
#include <memory>

// Simple mapping: MIDI note number -> script file.
using NoteToScriptMap = std::unordered_map<int, juce::File>;

struct LoadedConfig
{
    juce::File configDir;     // ~/Library/Application Support/PythonHooker
    juce::File confFile;      // event.conf
    juce::File scriptsDir;    // scripts/
    NoteToScriptMap map;      // note -> script
    juce::String error;       // empty on success; may contain warnings too
};

// POC config loader:
//
// Config directory (macOS):
//   ~/Library/Application Support/PythonHooker/
//
// event.conf format:
//   note.60 = scripts/middle_c.py
//   note.61 = scripts/csharp.py
//
class ConfigLoader
{
public:
    static LoadedConfig loadDefaultConfig();

private:
    static juce::File defaultConfigDir();
    static LoadedConfig loadFromDir(const juce::File& configDir);

    static void parseEventConf(const juce::File& confFile,
                               const juce::File& baseDir,
                               NoteToScriptMap& outMap,
                               juce::String& inOutError);
};
