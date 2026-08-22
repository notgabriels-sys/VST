# Granular Freeze - Product Specification

## Status of this document

This describes the intended granular product, not only the current engineering
implementation. Internal version `0.1.2` is a stereo freeze/looper prototype
with a ten-second capture buffer, a requested 50 ms-10 s held window clamped to
available history, crossfaded looping, Freeze, 0.5x-2.0x pitch-rate control,
and a 1-500 ms crossfade control.

The exact older `v0.1.1-rc.1` private-draft assets passed AU validation and a
limited Ableton Live 12.4.2 AU/VST3 functional smoke test. That candidate
predates Hold Length, so current Hold behavior has not yet been validated in a
DAW. Subjective musical/sound-quality approval also remains unverified.

The prototype does not yet have a granular engine, independent time-stretch,
grain size/density/position controls, feedback, scatter, presets, a waveform
display, or performance slots. It is not a production-ready paid product.

## Intended product

### Summary

- Live performance and sound-design plugin that captures incoming audio and
  resynthesizes it with a granular engine.
- Focus on immediate, hands-on use in Ableton Live and Bitwig.

### Core features

- Freeze/hold with an adjustable 50 ms-10 s requested held window, clamped to
  available capture history
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

The workflows support cross-platform builds and validated archives, and prior
baseline revisions have exercised that path. Fresh macOS and Windows CI/archive
verification for the exact current revision is still pending. The older exact
draft assets met limited Ableton AU/VST3 loading and functional smoke criteria,
but the current Hold revision still requires exact-artifact host validation.
The prototype cannot meet audible granular-playback acceptance because it has
no granular engine. Subjective sound approval, credential-tested production
signing/notarization, a documented JUCE distribution basis, and public-release
approval also remain open.

## Commercial planning

The original proposal considered a EUR 29 early-access price and EUR 49 full
launch, with Gumroad license delivery. These are planning notes, not approval
to list, sell, or publish the prototype. Commercial distribution requires a
documented JUCE licensing basis, completed product scope, production signing,
DAW validation, final pricing and listing approval, and manual upload of the
approved files through Gumroad's authenticated product Content UI.
