# Omniarp

MIDI arpeggiator plugin by Ticco Ross. JUCE 8, VST3 + AU (Mac), VST3 (Windows), VST3 + LV2 (Linux).

## Controls
- **HOLD**: latches the chord so the arp keeps playing after you release the keys
- **ALGORITHM**: Up, Down, Up/Down, Foldback, Up Skip, Down Skip, Pedal Up, Pedal Down, Pedal Rand, Rand, Chord
- **GATE**: note length (0-1 of a step)
- **SWING**: delays every second step
- **NOTE**: rate, 1/4 to 1/32 incl. dotted and triplets
- **NOTE RPT**: repeat each note 1-5x
- **OCT**: 0, +1, +2, +3, -1, -/+1
- **OCT RPT**: cycles per octave before moving on (1-5)

Click the "OMNIARP" logo for the About panel.

## Build
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release

Linux packages: libasound2-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxext-dev libfreetype6-dev libfontconfig1-dev

The manual is in Docs/Omniarp-Manual.pdf (rebuild with `python3 Docs/make_manual.py Omniarp-UI.png Docs/Omniarp-Manual.pdf`).

Pushing to GitHub runs `.github/workflows/build.yml` for Mac, Windows and Linux; tagging `v1.0.0` publishes a release with the installers.
