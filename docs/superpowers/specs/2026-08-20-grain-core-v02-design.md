# Granular Freeze v0.2: Grain Core Design

**Status:** Approved by Gabriel

**Date:** 2026-08-20

**Target repository:** `notgabriels-sys/VST`

**Target version:** `0.2.0`

## 1. Decision

Version 0.2 will replace the prototype's frozen-loop playback with a real,
deterministic granular engine. The product remains a compact live-input effect:
capture sound, press one Freeze control, and immediately shape a stable texture
with Size, Density, Position, and Pitch.

The milestone is intentionally narrow. It establishes a trustworthy grain core
and a playable six-control interface. It does not attempt a preset ecosystem,
random modulation, waveform editing, licensing, or a commercial release.

This is an architectural change because it introduces a new DSP subsystem and
changes frozen playback while preserving the existing plug-in interface and
session state.

## 2. Product intent

The intended user is a producer or live performer who wants to turn the most
recent incoming sound into a granular texture without loading a sample or
navigating a complex synthesizer. The differentiator is not feature count. It
is immediate, deterministic control of live audio in a compact cross-DAW
effect.

The product position is informed by established granular tools such as
[Ableton Granulator III](https://www.ableton.com/en/packs/granulator-iii/),
[Output Portal](https://shop.output.com/products/portal), and
[Arturia Efx FRAGMENTS](https://support.arturia.com/hc/en-us/articles/4494280268306-Efx-FRAGMENTS-General-Questions).
Those products already cover broad modulation, preset, and performance feature
sets. v0.2 therefore concentrates on the smallest useful claim Granular Freeze
can prove well: a fast, stable, deterministic live-input freeze.

No price, licence model, sales channel, or revenue forecast is approved by this
design. The speculative pricing in `docs/PRODUCT_SPEC.md` is not a v0.2
decision. Gabriel retains the pricing and release decision.

## 3. Goals and success criteria

v0.2 succeeds when all of the following are true:

- Live mode remains transparent stereo pass-through while continuously
  capturing up to eight seconds of valid input.
- Engaging Freeze stops capture and starts granular playback immediately when
  Density is above zero.
- Size, Density, Position, and Pitch produce distinct, deterministic, audible
  behavior.
- `position = 1.0` addresses the newest complete grain window rather than
  starting a loop from the oldest captured audio.
- Identical stereo input remains sample-aligned through every grain event.
- Empty, very short, silent, and automated inputs remain finite and stable.
- The audio thread performs no heap allocation, file access, locking, or
  unbounded work.
- The existing macOS and Windows build/test/package paths remain green.
- Gabriel can load the result in Ableton Live or Bitwig and complete the human
  listening gate described in section 13.

Passing automated checks does not by itself make v0.2 release-ready.

## 4. Scope

### Included

- A new isolated `GrainEngine` DSP component.
- A fixed pool of 64 grain voices.
- Deterministic grain scheduling.
- Hann grain envelopes.
- Cubic-interpolated pitched source reads.
- Chronological addressing of the frozen circular capture.
- Envelope-weighted overlap normalization.
- Three new automatable parameters: Size, Density, and Position.
- A compact editor exposing all six v0.2 parameters.
- Unit, processor-level, state-restoration, and listening-render coverage.
- Documentation reconciled with the behavior actually shipped in v0.2.

### Explicit non-goals

- Random spray, scatter, jitter, or probability.
- Feedback or recursive grain capture.
- Time stretching independent of pitch.
- Adjustable capture length; the existing eight-second capacity remains.
- Presets, quick slots, or a performance bank.
- A waveform, playhead, grain overlay, or modulation matrix.
- MPE, plug-in MIDI input, or an internal MIDI-learn system.
- Mono-to-stereo generation or additional bus layouts; v0.2 remains a stereo
  in-place effect.
- Signing, notarization, installers, store integration, licensing, pricing,
  tagging, or release publication.

These exclusions keep the milestone small enough for one implementation plan
and make listening feedback actionable before creative complexity is added.

## 5. Parameter contract

All parameters use JUCE `AudioProcessorValueTreeState` and remain host
automatable. Existing parameter IDs, ranges, defaults, and meanings are
preserved so older sessions and automation lanes do not silently change.

| ID | Display name | Range | Default | v0.2 behavior |
| --- | --- | --- | --- | --- |
| `freeze` | Freeze | off/on | off | Selects live capture or frozen granular playback. |
| `pitch` | Pitch | `0.50` to `2.00` ratio, step `0.01` | `1.00` | Source-read increment for grains; output grain duration does not change. |
| `crossfadeMs` | Crossfade | `1` to `500` ms, step `1` | `30` ms | Transition time between live and granular output. |
| `grainSizeMs` | Size | `5` to `200` ms, step `1` | `80` ms | Output duration and Hann-window length of grains launched after the change. |
| `densityHz` | Density | `0` to `200` grains/s, step `1` | `20` grains/s | Launch rate. At exactly zero, no grains launch and settled frozen output is silent. |
| `position` | Position | normalized `0.00` to `1.00`, step `0.01` | `1.00` | Grain start within valid captured history, oldest to newest complete window. |

The Size minimum is 5 ms for this stability-first milestone. The older product
document's 0.5 ms proposal is deferred because sub-millisecond events behave
more like impulses than dependable first-release grains. The existing Pitch
ratio is retained; changing it to a semitone range would alter automation
meaning and requires a separate migration design.

Parameter values are read once per processing block and clamped again at the
DSP boundary. Size, Position, and Pitch are captured by each voice when that
voice launches; changing them affects new grains, not active grains. Density
changes the scheduler interval from the next block. This makes automation
deterministic and avoids changing a voice's duration or source trajectory while
it is active. Crossfade is captured when a live/granular transition starts;
changing `crossfadeMs` affects the next transition rather than resizing one in
progress. Freeze changes take effect at the start of the next processing block.

## 6. Component architecture

### `GranularFreezeAudioProcessor`

The processor remains responsible for host integration, APVTS state, live
capture, freeze transitions, and dry/wet blending. It owns:

- the existing stereo circular capture buffer;
- capture metadata (`writePosition` and `validSamples`);
- a `GrainEngine` instance;
- a preallocated stereo wet-render scratch buffer; and
- continuous live/granular crossfade state.

The processor caches raw APVTS parameter pointers outside `processBlock`.
`prepareToPlay` allocates and clears the capture and scratch buffers, prepares
the engine, and resets all capture, scheduler, voice, and transition state.
If a host supplies a block larger than the prepared scratch capacity, the
processor handles it in bounded scratch-sized chunks instead of allocating.
Capture and transition state advance in a sample-major loop shared by both
channels; no timeline index may advance once per channel.

### `GrainEngine`

`src/GrainEngine.h` and `src/GrainEngine.cpp` form an isolated DSP unit. The
engine receives a read-only frozen-buffer view, current launch parameters, and
a destination region to render. It does not own, resize, or copy the eight-
second capture.

Its state consists of:

- a fixed `std::array` of 64 grain voices;
- a deterministic next-launch countdown;
- the prepared sample rate; and
- bounded indices/counters needed for voice selection and diagnostics.

Each voice stores only launch-time state: active flag, logical source position,
source increment, envelope position, envelope length, and launch age/order.
The same voice event and logical read position are used for both channels.
Voice timing advances once per output sample, never once per channel.

If all 64 voices are active, the engine deterministically replaces the oldest
voice. At the maximum specified Size and Density, normal operation needs at
most 40 simultaneous voices, so the pool has deliberate headroom. Voice
replacement remains defined for pathological automation or unsupported host
conditions.

### Frozen-buffer view

At Freeze engagement the processor records an immutable logical view:

- `span`: the current `validSamples`;
- `oldestPhysicalIndex`: `writePosition` when the capture is full, otherwise
  zero; and
- the circular buffer's fixed physical capacity.

Logical position zero is always the oldest valid captured sample. A logical
sample index maps to physical storage as:

```text
physical = (oldestPhysicalIndex + wrappedLogicalIndex) % capacity
```

Cubic interpolation resolves all four neighboring samples through this mapping
and wraps only inside `span`, never through uncaptured zero-filled storage.
This removes the prototype's ambiguity between physical buffer order and the
chronological audio heard by the performer.

## 7. Grain behavior

### Launch scheduling

When Freeze changes from off to on, the engine resets active voices and timing.
If `densityHz > 0`, it launches the first grain on the first frozen sample, then
uses a fractional sample countdown with interval:

```text
launchIntervalSamples = sampleRate / densityHz
```

The interval is clamped to at least one sample. At most one event may launch on
an output sample, so scheduler work is bounded. A Density change while running
clamps the remaining countdown to the new interval; it never creates a backlog
or burst. Moving Density from zero to a positive value launches immediately.
At zero, active voices finish naturally and no new voices launch.

No random source is used. Identical input, state, automation, sample rate, and
block sequence must produce identical output.

### Size and envelope

For a grain launched with `grainSizeMs`, output duration is:

```text
grainSamples = max(2, round(grainSizeMs * 0.001 * sampleRate))
```

Its sample at envelope index `n` is weighted by a Hann window:

```text
0.5 - 0.5 * cos(2*pi*n/(grainSamples - 1))
```

The window starts and ends at zero. Once the final envelope sample is rendered,
the voice becomes inactive.

### Pitch and source span

Pitch is the existing ratio and becomes the source-read increment. A ratio of
`2.0` consumes source twice as quickly and raises pitch by approximately one
octave; `0.5` consumes it half as quickly and lowers pitch by approximately one
octave. Grain output duration remains set by Size.

The source samples required for a complete grain are:

```text
sourceSpan = 1 + ceil((grainSamples - 1) * pitch)
```

### Position

When `span >= sourceSpan`, the latest legal complete-grain start is
`span - sourceSpan`. The launch start is:

```text
logicalStart = round(position * (span - sourceSpan))
```

Therefore Position 0 selects the oldest complete window and Position 1 selects
the newest complete window, not the raw write boundary.

When `0 < span < sourceSpan`, no complete non-wrapping window exists. In that
case Position maps from logical sample zero through `span - 1`, and pitched
reads wrap inside the valid span. Hann endpoints keep the grain output smooth.
When `span == 0`, no voice launches and the wet output is zero.

All interpolation indices and parameter-derived lengths are bounded before
use. Silent input remains silent. Invalid floating-point output is replaced by
zero as a last safety boundary, while tests must identify any path that reaches
that guard.

### Overlap normalization

For every output sample, the engine sums each active voice's windowed sample per
channel and separately sums the active Hann weights. It divides channel sums by
`max(1.0, weightSum)`. This preserves single-grain fades while preventing gain
from growing directly with overlap count. The same normalization denominator is
used for both stereo channels.

## 8. Processing data flow

### Live state

1. Input is copied to output unchanged.
2. Each input sample is written to the circular capture.
3. `writePosition` advances and `validSamples` grows to the fixed capacity.
4. The wet scratch buffer and grain engine remain inactive/reset.

### Freeze engagement and frozen state

1. Capture stops before the first frozen sample, making the captured region
   immutable.
2. The processor snapshots the chronological frozen-buffer view.
3. Grain voices and scheduler reset.
4. The engine renders wet audio into the preallocated scratch buffer.
5. The processor crossfades from the unchanged live input to the wet render.
6. Once settled, output is the normalized grain render.

### Unfreeze and rapid reversal

On Unfreeze, the processor crossfades from the immutable grain render back to
live input. Capture resumes only after the transition reaches fully live, so
the source cannot be overwritten while fading away. The next captured sample
continues at the existing write position. Grain voices continue rendering
during the fade, then reset when the fully-live endpoint is reached.

Freeze automation may reverse direction before a transition completes. A new
transition starts at the current live/granular blend coefficient rather than
jumping to either endpoint. A full-scale zero-to-one transition takes
`crossfadeMs`; a reversal covers only the remaining coefficient distance and
takes that same fraction of `crossfadeMs`. Each segment uses a scaled
half-cosine curve with exactly matching start amplitude. If Unfreeze reverses
before reaching fully live, capture has not resumed, so the same immutable
snapshot and active grain engine can continue safely.

## 9. Real-time safety contract

The following operations are forbidden from the audio callback:

- heap allocation, buffer resize, or container growth;
- file, network, console, or logging I/O;
- mutexes, waits, or blocking calls;
- random-device access; and
- loops whose iteration count can grow with elapsed scheduler debt.

All buffers and the voice pool are prepared before playback. Work per sample is
bounded by 64 voices and two channels. Parameter atomics are read once per block
and passed to the engine as plain clamped values. The design is lock-free, but
the implementation still requires structural review because ordinary unit
tests alone cannot prove real-time safety.

## 10. State and compatibility

- The APVTS root remains `PARAMS`.
- Existing IDs `freeze`, `pitch`, and `crossfadeMs` remain unchanged.
- New IDs are exactly `grainSizeMs`, `densityHz`, and `position`.
- Loading v0.1 state restores the three old values and supplies v0.2 defaults
  for missing new values.
- Saving and reloading v0.2 state round-trips all six values.
- Unknown future properties must not crash or corrupt current parameter state.
- The plug-in code version advances to `0.2.0`, but this milestone does not
  create a Git tag or public release.

Compatibility here means parameter/session compatibility and the existing
stereo AU/VST3 build targets. Bit-identical v0.1 frozen-loop audio is not a goal;
frozen playback is intentionally replaced by granular playback.

## 11. Editor design

The editor remains compact and uses standard JUCE controls and APVTS
attachments. It presents:

- a prominent Freeze toggle;
- Pitch and Position as the primary source controls;
- Size and Density as the primary texture controls; and
- Crossfade as the transition control.

Every continuous control shows its unit or normalized value and has a visible
label. The layout may grow vertically from the current 420 x 160 prototype but
must remain usable without tabs, menus, scrolling, or a preset browser. The
title no longer labels the plug-in as a prototype.

Host automation exposes every parameter. MIDI mapping is supplied by the host
(for example Ableton Live or Bitwig), not by adding plug-in MIDI input in v0.2.

## 12. Verification design

### Isolated `GrainEngine` tests

- Empty and silent views render silence with no NaN or Inf.
- A captured non-silent source produces non-silent grain output.
- Grain duration and Hann endpoints follow Size at multiple sample rates.
- Deterministic launch counts follow Density over a known render duration.
- Density zero launches nothing; changing from zero to positive launches once
  immediately and then follows the interval.
- Position selects distinguishable old and recent segments in a deliberately
  segmented capture, including a physically wrapped circular buffer.
- Pitch `2.0` and `0.5` produce the expected approximate frequency ratios.
- Identical stereo sources remain sample-identical, proving that voice state
  advances once per sample rather than once per channel.
- Maximum Size and Density remain bounded by the voice pool and produce finite
  output.
- Repeated identical renders are sample-identical.

### Processor integration tests

- Unfrozen stereo pass-through remains unchanged.
- Freeze holds captured audio when live input becomes silent.
- Freeze and Unfreeze transitions remain click-controlled relative to the dry
  source's own maximum sample step.
- Rapid transition reversal starts from the current blend with no endpoint
  jump.
- Very short capture, empty capture, and block-boundary Freeze are safe.
- Parameter sweeps never produce NaN or Inf.
- State restored both before and after `prepareToPlay` follows the same
  parameter/default contract.
- v0.1-shaped state receives new defaults; v0.2 state round-trips all values.
- Existing sample-rate and block-size coverage remains in CI.
- With static parameters, splitting the same input into different supported
  block sizes produces equivalent scheduler timing and output within floating-
  point tolerance.
- An oversized process block is handled in bounded chunks without allocation.

### Listening renders

`RenderDemo` gains named WAV cases that isolate:

- short and long Size;
- low and high Density;
- oldest, middle, and newest Position;
- octave-down, unity, and octave-up Pitch; and
- normal, short, and long transitions.

The renderer continues to report objective peak, RMS, DC, finite-value, stereo,
and maximum-step measurements. Rendered WAVs are listening aids, not CI
pass/fail evidence and not proof of musical quality.

### Build and CI

The existing macOS and Windows workflows must compile the plug-in, isolated
engine tests, processor tests, and renderer. Packaging must still produce the
same platform artifact types. No release workflow run is required by this
milestone.

## 13. Human listening gate

After implementation, local tests, and CI pass, Gabriel must audition a build
in Ableton Live or Bitwig before any release, tag, pricing, or sales work.
The listening pass must check:

- whether the default 80 ms / 20 grains-per-second texture is immediately
  useful;
- whether Position 1 feels like freezing the most recent sound;
- whether Size, Density, and Pitch ranges are musically useful;
- whether freeze, unfreeze, and rapid toggles click or pump;
- whether stereo material remains stable; and
- whether CPU use is acceptable in a realistic live set.

Findings from that pass may change defaults or reveal defects. They do not
automatically authorize the deferred feature set or a public release.

## 14. Documentation changes included with implementation

Implementation will update `README.md`, `TODO.md`, and
`docs/PRODUCT_SPEC.md` so they distinguish verified v0.2 behavior from future
ideas. In particular, the old six-week release schedule, speculative pricing,
feedback, scatter, presets, waveform UI, and broad acceptance claims must not
read as completed or approved v0.2 commitments.

Build, test, and release documentation changes are limited to commands or
behavior that actually change during implementation. Release instructions may
remain documented, but release execution remains outside this milestone.

## 15. Completion boundary

The v0.2 implementation is complete only when code, tests, listening renders,
documentation, and remote CI agree with this specification. At that point the
result is a tested release candidate for Gabriel's listening pass. It is not a
published product, a validated commercial offer, or proof of income potential.
