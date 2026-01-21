# PythonHooker VST

PythonHooker is a proof-of-concept VST3 MIDI-effect plugin that treats MIDI events as triggers for external Python scripts.

The plugin produces no audio. Its purpose is to intercept MIDI events inside a DAW and fire configurable, side-effect-only tasks via Python.

This project is intentionally minimal and explicit, designed to validate the core idea before adding features like MIDI pass-through, live config reload, or long-running Python daemons.

----------------------------------------------------------------

PROJECT STATUS

Implemented (POC)
- VST3 MIDI Effect built with JUCE and CMake
- Loads configuration from disk
- Maps MIDI note-on events to Python scripts
- Executes Python scripts asynchronously (off the audio thread)
- Tested in REAPER on Linux

Not yet implemented
- MIDI re-emission / proxying
- Live config reload
- Long-running Python daemon
- GUI editor for mappings
- macOS signing / notarization
- Windows installer

----------------------------------------------------------------

ARCHITECTURE OVERVIEW

MIDI event (DAW playback or live input)
    ↓
PythonHooker VST (audio thread)
    ↓
Lock-free queue
    ↓
Background worker thread
    ↓
Python script executed via OS

Design constraints:
- The audio thread is never blocked
- Python is never executed on the audio thread
- Script execution is best-effort and side-effect-only

----------------------------------------------------------------

CONFIGURATION

The plugin reads configuration from the user’s application data directory.
The exact path in use is displayed in the plugin UI.

Config directory by platform:

- Linux:
  ~/.config/PythonHooker/

- macOS:
  ~/Library/Application Support/PythonHooker/

- Windows:
  %APPDATA%\PythonHooker\

Required layout:

PythonHooker/
  event.conf
  scripts/
    middle_c.py

----------------------------------------------------------------

EVENT.CONF FORMAT

event.conf maps MIDI note numbers to Python scripts.

Example:

  note.60 = scripts/middle_c.py

Rules:
- Keys are note.<0–127>
- Values are paths relative to the config directory
- One script per note (POC limitation)

----------------------------------------------------------------

PYTHON SCRIPT CONTRACT

Python scripts are invoked with command-line arguments describing the MIDI event.

Arguments passed:
- --note   MIDI note number (0–127)
- --vel    Velocity (0–127)
- --chan   MIDI channel (1–16)
- --time   Host time in seconds

Recommended POC script (writes to a log file instead of stdout):

  import argparse
  import time

  p = argparse.ArgumentParser()
  p.add_argument("--note", type=int, required=True)
  p.add_argument("--vel", type=int, required=True)
  p.add_argument("--chan", type=int, required=True)
  p.add_argument("--time", type=float, required=True)
  args = p.parse_args()

  with open("/tmp/pythonhooker.log", "a") as f:
      f.write(
          f"{time.time():.3f} "
          f"HOOK note={args.note} vel={args.vel} "
          f"chan={args.chan} hosttime={args.time}\n"
      )

Important:
- Printing to stdout is unreliable in DAW/plugin contexts
- Always produce a visible side effect (file, IPC, OSC, network)

----------------------------------------------------------------

BUILDING THE PLUGIN

The project uses JUCE and CMake and builds a VST3 plugin.

Prerequisites (all platforms):
- CMake 3.22 or newer
- C++17-capable compiler
- JUCE (included as a submodule)
- Python 3 available at runtime

----------------------------------------------------------------

LINUX (DEBIAN / UBUNTU)

Dependencies:

  sudo apt install -y \
    build-essential cmake git pkg-config \
    libasound2-dev libx11-dev libxext-dev \
    libxrandr-dev libxinerama-dev libxcursor-dev \
    libfreetype6-dev libgtk-3-dev

Build:

  git submodule update --init --recursive
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build -j

Install (user-local):

  mkdir -p ~/.vst3
  cp -r build/*_artefacts/Release/VST3/PythonHooker.vst3 ~/.vst3/

----------------------------------------------------------------

MACOS

Prerequisites:
- Xcode or Xcode Command Line Tools
- CMake
- Python 3

Build:

  git submodule update --init --recursive
  cmake -S . -B build -G "Xcode" -DCMAKE_BUILD_TYPE=Release
  cmake --build build --config Release

Install:

  cp -R build/*_artefacts/Release/VST3/PythonHooker.vst3 \
        ~/Library/Audio/Plug-Ins/VST3/

----------------------------------------------------------------

WINDOWS

Prerequisites:
- Visual Studio 2022 with C++ workload
- CMake
- Python 3

Build (Developer Command Prompt):

  git submodule update --init --recursive
  cmake -S . -B build -G "Visual Studio 17 2022" -A x64
  cmake --build build --config Release

Install:

  Copy PythonHooker.vst3 to:
  C:\Program Files\Common Files\VST3\

----------------------------------------------------------------

USAGE (REAPER TEST FLOW)

1. Launch REAPER
2. Set Audio System to Dummy Audio (Options → Preferences → Audio → Device)
3. Insert a track
4. Insert PythonHooker (VST3) as a track FX
5. Insert a MIDI item on the same track
6. Add a MIDI note matching event.conf (e.g. note 60)
7. Press Play
8. Verify /tmp/pythonhooker.log is created and appended to

Success criteria:
- Playback cursor moves
- MIDI item plays
- Python script runs
- Log file updates

----------------------------------------------------------------

LICENSE

MIT (or project-specific license to be added)

----------------------------------------------------------------

END
