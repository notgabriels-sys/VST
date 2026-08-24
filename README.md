# Granular Freeze

Granular Freeze v0.2.0 is a compact stereo AU/VST3 live-input effect.

It builds VST3 for macOS and Windows and AU for macOS. Repository-authored
source is MIT-licensed. Binary builds incorporate JUCE 8.0.15, which is
separately dual-licensed under AGPLv3 or the commercial JUCE 8 licence. No
distribution basis has been recorded for this product, so production binaries
must not be published or sold. See [docs/RELEASE.md](docs/RELEASE.md).

**Implemented and automatically tested. The exact v0.2.0 VST3 has been
discovered and instantiated in Ableton Live 12 and Bitwig Studio; its Ableton
editor opened. Manual auditory QA and session-recall QA were waived, not
passed. It is not publicly released and has no commercial validation.**
Automated tests and host loading do not establish musical quality, CPU
suitability, state recall, a release decision, pricing, or sales readiness.

## Implemented Grain Core

- Transparent stereo live pass-through records a chronological circular capture
  of up to ten seconds.
- Hold selects the most recent 50 ms–10 s chronological window, clamped to
  available capture, and latches that view when Freeze engages.
- Freeze renders that held view through a deterministic fixed 64-voice
  engine with Hann windows, cubic pitched reads, and overlap normalization.
- Position 0.00 selects the oldest complete grain window; 1.00 selects the
  newest complete window, not the raw circular-buffer write location.
- Freeze/Unfreeze transitions are reversible from the current blend.
- v0.1 state receives Hold and Grain Core defaults; v0.1.2 state retains Hold
  while receiving Grain Core defaults; v0.2 state round-trips all seven values.
  Tests cover migration, automation, finite guards, and chunking.

## Controls

All seven AudioProcessorValueTreeState parameters are host-automatable.

| Host ID | Control | Range | Default |
| --- | --- | --- | --- |
| freeze | Freeze | off / on | off |
| pitch | Pitch | 0.50–2.00 ratio, 0.01 step | 1.00 |
| crossfadeMs | Crossfade | 1–500 ms, 1 ms step | 30 ms |
| holdMs | Hold | 50–10,000 ms, 1 ms step | 1,000 ms |
| grainSizeMs | Size | 5–200 ms, 1 ms step | 80 ms |
| densityHz | Density | 0–200 grains/s, 1 grain/s step | 20 grains/s |
| position | Position | 0.00–1.00, 0.01 step | 1.00 |

Size determines new-grain duration. Density is a deterministic launch rate and
zero settles frozen output to silence. Pitch is source-read rate. The editor
exposes Freeze, Pitch, Position, Size, Density, Hold, and Crossfade.

## Evidence and deferred scope

The offline GranularFreezeEngineTests and GranularFreezeTests executables need
no host/device. GranularFreezeRender writes fifteen controlled Freeze cases
plus dry-reference.wav: exactly sixteen stereo 48 kHz/24-bit WAV listening
aids. The hardened `main` commit
`4b7eeee17db72b03e7aa8f33e6f9b215c51c5279` completed the macOS and Windows
CI matrix, including both test binaries, packaging, and artifact upload. The
downloaded artifacts passed checksum, archive-integrity, and expected-payload
checks; the macOS AU and VST3 also have valid ad-hoc seals, universal
arm64/x86_64 binaries, and a macOS 12.0 minimum deployment target. Apple
`auval` passed for the exact installed AU. Ableton Live 12 discovered both
formats and loaded the VST3 editor; Bitwig Studio identified and instantiated
the Arm64 VST3 and exposed its parameters. This is engineering and host-load
evidence, not auditory, recall, signing, or commercial-release evidence.

The hardened candidate path uses pinned macOS and Windows runners, a universal
macOS 12 build, strict archive scripts, fail-closed conditional signing and
notarization, signing-status manifests, and private draft prereleases. No
credential-dependent signing path has executed. Those paths still require
remote execution and downloaded-asset inspection; their presence in the
repository is not release evidence.

The exact `v0.2.0-rc.1` artifacts are staged in a private, unpublished GitHub
draft with verified SHA-256 read-back. They are ad-hoc/unsigned engineering
artifacts and must not be sold or publicly published.

Time-stretch, presets/performance banks, feedback, random scatter/modulation,
waveform UI, credential-tested production signing/notarization, store work, and
all commercial/release decisions are deferred, not current product claims. See
[docs/BUILD_AND_TEST.md](docs/BUILD_AND_TEST.md) for exact checks and human
listening boundaries.

---

<!-- catalog-footer -->
Part of **Gabriel Tools + Code** — a public catalog of audio products, studio utilities, design systems, software and repositories by Gabriel García Alonso:

**[Open the full catalog →](https://gabriel-tools-and-code.notgabriels960914.chatgpt.site)**

Related free tools: [theme-contrast](https://github.com/notgabriels-sys/theme-contrast) · [htmlshot](https://github.com/notgabriels-sys/htmlshot) · [50 dark themes for Claude Code](https://github.com/notgabriels-sys/claude-code-50-dark-themes).
