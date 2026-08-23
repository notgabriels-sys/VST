# Granular Freeze v0.2.0 — Product Specification

## Status

**Implemented and automatically tested. Not yet evaluated by Gabriel in Ableton
Live or Bitwig. It is not released. No commercial validation.** This is a factual
implementation scope, not a price, schedule, sales-channel, compatibility, or
musical-quality claim. Renderer diagnostics and WAV structure are not proof of
quality.

## Implemented behavior

Live mode passes stereo audio through while recording a chronological circular
capture of up to eight seconds. Freeze snapshots valid history, stops capture,
and crossfades to granular output; returning live crossfades back before capture
resumes. The deterministic Grain Core has a fixed 64-voice pool, Hann
envelopes, cubic-interpolated pitched reads, and envelope-weight normalization.
Position 0.00 selects the oldest complete source window and 1.00 the newest.
Empty capture launches no grains; short capture uses only its valid span.

Transitions reverse from the current blend. Parameters are read/clamped once
per block; preallocated scratch storage chunks oversized blocks and finite
guards protect the DSP boundary. These are automated-test facts, not real-session
CPU evidence.

## Parameter contract

All values are host-automatable APVTS parameters.

| ID | Name | Range/step | Default |
| --- | --- | --- | --- |
| freeze | Freeze | off / on | off |
| pitch | Pitch | 0.50–2.00, 0.01 | 1.00 |
| crossfadeMs | Crossfade | 1–500 ms, 1 ms | 30 ms |
| grainSizeMs | Size | 5–200 ms, 1 ms | 80 ms |
| densityHz | Density | 0–200 grains/s, 1 grain/s | 20 grains/s |
| position | Position | 0.00–1.00, 0.01 | 1.00 |

Pitch, Size, and Position are captured at voice launch. Density never creates
a backlog; zero-to-positive launches immediately. The compact editor exposes
all six controls.

## State, evidence, and human gate

The project version is 0.2.0. APVTS root PARAMS and IDs freeze, pitch, and
crossfadeMs are retained. v0.1 state supplies defaults for grainSizeMs,
densityHz, and position; v0.2 state round-trips all six values.

GranularFreezeEngineTests and GranularFreezeTests cover engine, processor,
state, automation, editor, transition, finite-output, and bounded-block cases.
GranularFreezeRender writes fourteen named 48 kHz/24-bit stereo WAVs. It fails
on file/I/O or non-finite conditions; metrics are diagnostics, not quality
gates.

Before tag, pricing, sales, or release decisions, Gabriel must listen in
Ableton Live or Bitwig: default texture, newest-window Position, Size/Density/
Pitch ranges, transitions, stereo stability, and realistic CPU use. Remote
macOS/Windows CI is separate from local verification; listening is separate
from both.

## Deferred, outside v0.2

- Time-stretch independent of pitch.
- Presets, performance banks, or example preset content.
- Feedback/recursive capture.
- Random scatter, jitter, probability, or other random modulation.
- Waveform, playhead, grain overlay, or modulation-matrix UI.
- Signing, notarization, installers, store work, pricing, tags, publication,
  and commercial validation.
