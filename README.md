# WY / GLITCH

A JUCE 8 macOS VST3/AU glitch effect for Ableton Live.

## Interface
Dark neon interface based on the approved WY / GLITCH visual concept, with four texture buttons (LIQUID, CRYSTAL, ORGANIC, PLASMA), four rotary controls (GLITCH, SPEED, CHAOS, MIX), an animated decorative waveform, and four factory preset selections. All controls are English.

The waveform is a visual animation, **not** a real-time audio waveform analyzer. The design approximates the concept illustration with native JUCE vector graphics, not a pixel-perfect reproduction.

## Audio
The four texture algorithms process real audio, with host-BPM-synced slicing controlled by SPEED. The BPM sync is automatic; the SPEED knob selects a rhythmic division. Presets set real parameters and are saved with the DAW session.

## Build
Upload the **contents** of this folder to the root of a new GitHub repository, including hidden `.github/workflows/macos.yml`. GitHub Actions > Build WY Glitch macOS > Run workflow (or push to main). Download the macOS artifact after the workflow succeeds.

## Install
Copy `WY Glitch.vst3` to `~/Library/Audio/Plug-Ins/VST3/` and `WY Glitch.component` to `~/Library/Audio/Plug-Ins/Components/`, then rescan plugins in Ableton. macOS may require quarantine removal or local re-signing of downloaded bundles.

## Status
Source package only; this environment has not compiled or validated the macOS AU/VST3 build or tested Ableton loading.
