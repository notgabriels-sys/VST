# Spectral animation directions

The editor's shell is now intentionally quiet. The next choice is the motion
language inside the wide screen. Every option below can use the same real-time
contract: fixed-size telemetry, no audio-thread allocations, no locks, no new
host parameters, and no added latency in the plug-in's audio path.

“0 ms lag” needs one precise distinction. The audio path can remain sample-
accurate and latency-free because the visual analyzer is a side readout; a
screen still needs a bounded analysis window and a display refresh. The options
below state those visual costs honestly rather than pretending a monitor can
present a new frame before the samples exist.

## Option 1 — Graphite Spectrum

The most literal and restrained direction. Use 32 logarithmically spaced bands,
a single thin contour, short meter columns, and a very small number of grain
glints only where current spectral energy exists. The contour attacks quickly,
then releases slowly enough to remain legible without smearing transients.

- Character: website-like, calm, precise, almost typographic.
- Visual cost: very low; the screen reads as a real spectrum, not an effect.
- Signal design: 128-sample analysis window, immediate attack, bounded release.
- Best for: the default product surface and long hypnotic sessions.

## Option 2 — Spectral Ribbon

Replace discrete columns with a high-resolution 48-band ribbon: a thin main
trace, a quieter one-pixel under-trace, and restrained peak ticks that fade
independently. Grain activity modulates the trace's local brightness rather than
creating free-floating particles.

- Character: most polished and high-end; fluid without becoming decorative.
- Visual cost: low; one coherent frequency object carries the whole animation.
- Signal design: 128-sample window, no audio-path delay, UI interpolation only.
- Best for: the strongest match to the website's flat graphite restraint.

## Option 3 — Quiet Waterfall

Render a narrow, low-contrast history of spectral contours moving horizontally
through the screen. The newest contour is the brightest; older contours become
near-black graphite. There is no particle layer, only time becoming visible as a
faint trail.

- Character: cinematic, deep, and more obviously “spectral.”
- Visual cost: moderate; the history must be kept bounded and carefully faded.
- Signal design: 128-sample analysis window plus a fixed 96-frame visual ring.
- Best for: a premium visual mode, not necessarily the only default mode.

## Option 4 — Resonant Grain Field

Keep the spectrum as a thin baseline, but let the actual grain voices appear as
small, sharply bounded glints at their measured spectral region. Glints inherit
the grain envelope and disappear when the voice ends; they never wander without
DSP evidence. The result connects the instrument's granular identity to a real
frequency display without turning the screen into a constellation.

- Character: mythic but disciplined; the most distinctive Granular Freeze mode.
- Visual cost: low to moderate; requires richer per-voice frequency telemetry.
- Signal design: 128-sample spectrum plus fixed voice-frequency metadata.
- Best for: a signature mode after the base spectrum is approved.

## Option 5 — Transient Lattice

Use the spectral flux between analysis frames to draw sparse vertical impulses.
Steady tones stay quiet; attacks and rhythmic events briefly articulate the
screen. A very thin low-frequency floor keeps the display alive between hits.

- Character: precise, percussive, and especially suited to hypnotic techno.
- Visual cost: low; motion is event-driven instead of continuously busy.
- Signal design: short-window flux detector with bounded peak decay.
- Best for: a performance-focused mode where timing matters more than a full
  continuous spectrum.

## Recommendation

Start with Option 2, Spectral Ribbon. It is the cleanest bridge between a real
frequency spectrum and the website's almost-flat graphite language. If the
instrument needs a more unmistakable granular signature afterward, Option 4 is
the strongest second experiment. Options 3 and 5 are credible alternate modes,
but they should earn their place through a listening-and-screen comparison
rather than being enabled by default immediately.

Option 2 is now the selected reference direction: the editor uses its 48-band
ribbon, independently decaying peak ticks, and a 128-sample visual analysis
window. Option 4 was rendered as a bounded comparison against the same
signal; its spectrum-derived glints were attractive but read more like a
diagnostic field than a continuous musical surface in this shell, so the
comparison renderer is not shipped or exposed as a second user-facing mode.
