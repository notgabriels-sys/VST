# Granular Freeze - Product Specification

## Status of this document

This describes the intended granular product, not the contents of the current
engineering candidate. Internal version `0.1.1` is a stereo freeze/looper
prototype. It has an eight-second capture buffer, crossfaded looping, Freeze,
0.5x-2.0x pitch-rate control, and a 1-500 ms crossfade control.

The candidate does not yet have a granular engine, independent time-stretch,
grain size/density/position controls, feedback, scatter, presets, a waveform
display, or performance slots. It has not been evaluated by ear in a DAW and
is not a production-ready paid product.

## Intended product

### Summary

- Live performance and sound-design plugin that captures incoming audio and
  resynthesizes it with a granular engine.
- Focus on immediate, hands-on use in Ableton Live and Bitwig.

### Core features

- Freeze/hold with adjustable 50 ms-10 s buffer
- Grain size 0.5-200 ms, density 0-200 grains/s, and buffer position
- Pitch range of +/-48 semitones
- Feedback and crossfade controls
- Continuous freeze-morph or scatter control
- Eight performance-recall slots and a preset system
- CPU-friendly defaults plus an optional higher-quality mode

### UI and presets

- Compact performance interface with a large Freeze button and direct grain,
  density, pitch, and position controls
- Buffer waveform with playhead and grain-density overlay
- MIDI-mappable controls
- Example banks for pads, percussion, ambience, drones, and field recordings

### Optional future work

- Multiband freezing
- Spectral-granular mode
- Ableton Link synchronization
- AAX format

## Intended acceptance criteria

- macOS and Windows builds produce VST3, plus AU on macOS.
- Freeze and actual granular playback are audible and stable at acceptable CPU
  settings.
- Exact release assets load in the target DAWs.
- CI produces validated archives for both platforms.
- Production artifacts are signed; macOS is notarized.

The current candidate meets only part of this list. Offline signal tests and a
successful build do not satisfy the audible granular-playback or DAW-host
criteria.

## Commercial planning

The original proposal considered a EUR 29 early-access price and EUR 49 full
launch, with Gumroad license delivery. These are planning notes, not approval
to list, sell, or publish the prototype. Commercial distribution requires a
documented JUCE licensing basis, completed product scope, production signing,
DAW validation, final pricing and listing approval, and manual upload of the
approved files through Gumroad's authenticated product Content UI.
