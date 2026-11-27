# John Pork Call (VST3)

A phone-call effect plugin. It starts out **ringing** (your audio passes through
untouched while the ringtone plays), then:

- **Accept** (green): phone-speaker filter (400 Hz to 3.4 kHz), grit, compression.
- **Decline** (red X): muffled, bit-crushed and quieter, like the line got cut.
- **Hang up** (middle button): back to ringing.

The **Intensity** knob sets how much distortion there is. The **Ringtone** switch
turns the ringtone on or off. The Call / Intensity / Ringtone controls can all be
automated in FL Studio.

## Files

- `background.png` is the call-screen picture, built into the plugin.
- `ringtone.wav` is the ringtone, built into the plugin.

To change either, replace the file with one of the same name and rebuild.

## Build with GitHub (no installs)

Upload every file in this folder to a GitHub repo, add `.github/workflows/build.yml`,
run it from the Actions tab, and download the `JohnPorkCall-VST3` artifact.

## Build on your own PC (Windows)

1. Install Visual Studio with the "Desktop development with C++" workload.
2. In Developer PowerShell, in this folder: `git clone https://github.com/juce-framework/JUCE.git`
3. `cmake -B build` then `cmake --build build --config Release`
4. Copy `build\JohnPorkCall_artefacts\Release\VST3\John Pork Call.vst3` into
   `C:\Program Files\Common Files\VST3`.
5. In FL Studio: Options > Manage plugins > Find plugins.

Not tested in a build here, so small compile errors are possible.
