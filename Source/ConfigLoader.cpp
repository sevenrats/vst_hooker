#include "ConfigLoader.h"

juce::File ConfigLoader::defaultConfigDir()
{
    // On macOS: ~/Library/Application Support/<AppName>/
    // JUCE userApplicationDataDirectory maps to that location on macOS.
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("PythonHooker");
}

LoadedConfig ConfigLoader::loadDefaultConfig()
{
    return loadFromDir(defaultConfigDir());
}

LoadedConfig ConfigLoader::loadFromDir(const juce::File& configDir)
{
    LoadedConfig cfg;
    cfg.configDir  = configDir;
    cfg.confFile   = configDir.getChildFile("event.conf");
    cfg.scriptsDir = configDir.getChildFile("scripts");

    if (! cfg.confFile.existsAsFile())
    {
        cfg.error = "Missing event.conf at: " + cfg.confFile.getFullPathName();
        return cfg;
    }

    if (! cfg.scriptsDir.exists())
    {
        // Not fatal, but likely wrong setup.
        cfg.error << "Warning: scripts directory does not exist: "
                  << cfg.scriptsDir.getFullPathName() << "\n";
    }

    parseEventConf(cfg.confFile, cfg.configDir, cfg.map, cfg.error);
    return cfg;
}

static bool parseNoteKey(const juce::String& lhs, int& noteOut)
{
    // Expected: note.<number>
    auto parts = juce::StringArray::fromTokens(lhs.trim(), ".", "");
    if (parts.size() != 2)
        return false;

    if (parts[0].trim().toLowerCase() != "note")
        return false;

    const auto numStr = parts[1].trim();
    if (! numStr.containsOnly("0123456789"))
        return false;

    int n = numStr.getIntValue();
    if (n < 0 || n > 127)
        return false;

    noteOut = n;
    return true;
}

void ConfigLoader::parseEventConf(const juce::File& confFile,
                                  const juce::File& baseDir,
                                  NoteToScriptMap& outMap,
                                  juce::String& inOutError)
{
    const auto content = confFile.loadFileAsString();
    auto lines = juce::StringArray::fromLines(content);

    for (auto line : lines)
    {
        line = line.trim();

        // Skip empty lines and comments (# or ;)
        if (line.isEmpty() || line.startsWithChar('#') || line.startsWithChar(';'))
            continue;

        const int eq = line.indexOfChar('=');
        if (eq < 0)
        {
            inOutError << "Warning: ignoring line without '=': " << line << "\n";
            continue;
        }

        const auto lhs = line.substring(0, eq).trim();
        const auto rhs = line.substring(eq + 1).trim();

        int note = -1;
        if (! parseNoteKey(lhs, note))
        {
            inOutError << "Warning: invalid key (expected note.<0-127>): " << lhs << "\n";
            continue;
        }

        if (rhs.isEmpty())
        {
            inOutError << "Warning: empty script path for " << lhs << "\n";
            continue;
        }

        // RHS is a path relative to baseDir (configDir), e.g. scripts/middle_c.py
        auto scriptFile = baseDir.getChildFile(rhs);

        if (! scriptFile.existsAsFile())
        {
            inOutError << "Warning: script not found for " << lhs << ": "
                      << scriptFile.getFullPathName() << "\n";
            continue;
        }

        outMap[note] = scriptFile;
    }

    if (outMap.empty())
        inOutError << "Warning: no mappings loaded from event.conf\n";
}
