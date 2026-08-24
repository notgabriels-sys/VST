# Granular Freeze v0.2.0 — Product Specification

## Status

**Implemented and automatically tested. Exact v0.2.0 host loading is confirmed
in Ableton Live 12 and Bitwig Studio; auditory QA and session recall were
waived, not passed. It is not publicly released and has no commercial
validation.** This is a factual implementation scope, not a price, schedule,
sales-channel, broad compatibility, or musical-quality claim. Renderer
diagnostics and host loading are not proof of quality.

## Implemented behavior

Live mode passes stereo audio through while recording a chronological circular
capture of up to ten seconds. On a fully live off-to-on Freeze transition, Hold
selects the most recent 50 ms–10 s of available history and latches that
chronological view; changing Hold while frozen applies on the next fully live
Freeze engagement. Freeze stops capture and crossfades to granular output;
returning live crossfades back before capture resumes. The deterministic Grain
Core has a fixed 64-voice pool, Hann
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
| holdMs | Hold | 50–10,000 ms, 1 ms, 0.4 skew | 1,000 ms |
| grainSizeMs | Size | 5–200 ms, 1 ms | 80 ms |
| densityHz | Density | 0–200 grains/s, 1 grain/s | 20 grains/s |
| position | Position | 0.00–1.00, 0.01 | 1.00 |

Pitch, Size, and Position are captured at voice launch. Density never creates
a backlog; zero-to-positive launches immediately. The compact editor exposes
all seven controls.

## State, evidence, and human gate

The project version is 0.2.0. APVTS root PARAMS and IDs freeze, pitch,
crossfadeMs, and holdMs are retained. AU version hints keep the original three
parameters in generation 1, Hold in generation 2, and Grain Core controls in
generation 3. v0.1 state receives defaults for Hold and the three Grain Core
controls; v0.1.2 state preserves Hold and receives only the Grain Core defaults;
v0.2 state round-trips all seven values.

GranularFreezeEngineTests and GranularFreezeTests cover engine, processor,
state, automation, editor, transition, finite-output, and bounded-block cases.
GranularFreezeRender writes sixteen named 48 kHz/24-bit stereo WAVs. It fails
on file/I/O or non-finite conditions; metrics are diagnostics, not quality
gates.

The repository also contains pinned macOS/Windows candidate workflows, strict
platform packaging scripts, a sanitized bundle identifier, fail-closed
conditional signing/notarization, signing-status manifests, and checksum/draft
release handling. These are engineering infrastructure, not evidence that the
workflow or any credential-dependent path has run for v0.2, that any candidate
asset exists, or that a release has been approved.

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
- Credential-tested production signing/notarization, installers, store work,
  pricing, tags, publication, and commercial validation.
