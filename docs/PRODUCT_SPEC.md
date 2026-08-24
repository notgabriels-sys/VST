# Granular Freeze 0.2.0 DPF product specification

## Status

DPF migration merged and cross-platform release-candidate builds verified. Not
released or commercially validated. Automated tests, native AU validation,
successful bundle creation, observed VST3 loading, dedicated third-party
validators, listening approval, signing, delivery, and publication remain
separate evidence gates.

## Processing contract

Live mode transparently passes and captures stereo input into a ten-second
circular history. A fully-live Freeze engagement latches the most recent Hold
window, stops capture, resets grain scheduling, and crossfades to deterministic
granular output. Unfreeze crossfades back before capture resumes. Reversals
start from the current blend.

The grain engine uses 64 fixed voices, Hann envelopes, cubic interpolation,
launch-time Size/Pitch/Position, deterministic Density scheduling, chronological
Position, and envelope-weight normalization. Empty capture remains silent;
invalid/non-finite boundary values are clamped or replaced with safe defaults.

## Host contract

The seven automatable parameters, in fixed order, are `freeze`, `pitch`,
`crossfadeMs`, `holdMs`, `grainSizeMs`, `densityHz`, and `position`. Their
ranges/defaults are documented in the README. The DPF release line uses DPF's
native host parameter persistence. Unpublished JUCE APVTS XML blobs are not a
supported cross-framework state format.

The plug-in is a stereo effect with no MIDI. Targets are VST3/CLAP on macOS and
Windows and AU on macOS. Product version is 0.2.0; CLAP ID is
`com.gabrielgarciaalonso.granularfreeze`; DPF IDs are `GF01`/`GFZP`.

## Deferred

Time-stretch independent of pitch, presets, feedback, random modulation,
waveform display, production signing/notarization, store setup, pricing,
publication, and commercial claims are outside this migration.
