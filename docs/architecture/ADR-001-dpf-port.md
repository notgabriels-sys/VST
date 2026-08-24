# ADR-001: Port Granular Freeze from JUCE to DPF

**Status:** Accepted
**Date:** 2026-08-24
**Deciders:** Gabriel Garcia Alonso

## Context

Granular Freeze 0.2 has a deterministic, tested JUCE implementation, but the
JUCE distribution boundary creates an unwanted commercial licensing cost. The
plug-in has not been publicly released. The port must preserve the musical and
automation contract while producing VST3, AU, and CLAP artifacts without a
framework subscription or per-release fee.

## Decision

Use DISTRHO Plugin Framework (DPF) at immutable commit
`4238e1c7f0351bbe488d79f0899c540543ac7583`, recursively fetching its tree-pinned
Pugl submodule commit `5e2621d714ddf1cb0f86e852f8ba5dffe04aa3a3`.

Split the product into three boundaries:

1. A framework-neutral C++ DSP core owns capture, freeze transitions, granular
   rendering, parameter sanitization, and bounded scratch storage.
2. A thin DPF `Plugin` adapter owns host parameter declarations, activation,
   sample-rate changes, and stereo buffer forwarding.
3. A DGL UI owns drawing and gestures only; DSP behavior remains testable
   without a graphics or plug-in framework.

The seven stable parameter symbols and their ranges/defaults remain:
`freeze`, `pitch`, `crossfadeMs`, `holdMs`, `grainSizeMs`, `densityHz`, and
`position`. DPF host state persists those parameters natively. The unpublished
JUCE APVTS XML blob is not declared binary-compatible with the DPF build;
because no public release exists, the DPF build becomes the first supported
session-state line.

## Options Considered

### DPF

| Dimension | Assessment |
| --- | --- |
| Complexity | Medium |
| Cost | No framework fee; attribution required |
| Formats | VST3, AU, CLAP from one codebase |
| Real-time control | Strong, small API surface |
| Migration risk | Host/state/UI rewrite; DSP can be preserved behind a neutral boundary |

### iPlug2

| Dimension | Assessment |
| --- | --- |
| Complexity | Medium |
| Cost | No framework fee under its permissive license |
| Formats | Broad plug-in format support |
| Team familiarity | None |
| Migration risk | Similar host/UI rewrite with a larger framework surface |

### Stay on JUCE

| Dimension | Assessment |
| --- | --- |
| Complexity | Low |
| Cost | Commercial distribution boundary remains |
| Compatibility | Existing candidate and state implementation |
| Migration risk | None technically, but fails the cost constraint |

## Trade-off Analysis

DPF minimizes licensing and framework surface while retaining the required
formats. The cost is a one-time adapter/UI rewrite and loss of unpublished JUCE
state-blob compatibility. Keeping DSP framework-neutral prevents this migration
from becoming another framework lock-in and makes offline tests fast enough for
strict TDD.

## Consequences

- VST3, AU, and CLAP can be distributed with the applicable ISC/MIT notices and
  no JUCE commercial plan.
- The JUCE candidate remains a private behavioral reference and is never
  relicensed or published as part of this decision.
- Plug-in identity and parameter symbols are stable from the DPF release line;
  hosts may still treat the DPF binary as a different implementation because
  wrapper-specific identifiers are framework-defined.
- Signing, Apple notarization, Windows signing, store fees, payment-provider
  fees, and taxes remain independent costs/gates; DPF removes only the
  framework-license pressure.
- AU/VST3/CLAP validation and DAW loading remain release gates after automated
  tests pass.

## Action Items

1. Extract and test a framework-neutral DSP core.
2. Add the DPF host adapter and DGL editor.
3. Build and validate VST3, AU, and CLAP artifacts on macOS and Windows CI.
4. Replace JUCE notices and release documentation with exact DPF attribution.
5. Keep all release publication disabled until Gabriel approves the DPF build.
