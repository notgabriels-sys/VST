Granular Freeze — Product Specification (v0.1)

Summary
- Live performance and sound-design plugin that captures a short buffer of incoming audio (freeze) and re-synthesizes it via a granular engine.
- Focus: immediacy and hands-on performance in Ableton Live and Bitwig. Fast controls for freeze, grain size, density, position, pitch, feedback and freeze morph.

Core features (initial)
- Freeze/hold toggle with adjustable buffer length (50ms — 10s)
- Grain Size (0.5ms — 200ms), Density (0 — 200 grains/s), Position (scrub within buffer)
- Pitch control with +/- 48 semitones (for creative shifts)
- Feedback and DPR (crossfade) to avoid clicks
- Freeze morph / continuous scatter control for evolving textures
- 8 quick slots for performance recall, full preset system
- CPU-friendly defaults + quality mode toggle for heavier use

UI / UX
- Compact, performance skin (large freeze button, encoders for grain/density/pitch/position)
- Visual buffer waveform with play-head and grain density overlay
- MIDI-mappable encoders and buttons for live control

Presets
- Example bank: "FreezePads", "GlitchPerc", "AmbientChains", "DroneFreeze", "FieldTextures"

Pricing & Licensing
- Intro price: €29 (early access) → €49 full launch
- Gumroad: single-license key delivery; optionally issue time-limited free beta keys

Milestones
- Week 1: Product spec, repo skeleton, CI + basic pass-through plugin (this stage)
- Week 2: Implement buffer + freeze control, basic granular playback (prototype)
- Week 3: UI controls, preset system, example presets
- Week 4: Beta testing, DAW compatibility testing (Ableton, Bitwig), performance tuning
- Week 5: CI packaging, signing, prepare marketing assets + demo audio/video
- Week 6: Release (GitHub Release + Gumroad)

Acceptance criteria
- Plugin builds on macOS and Windows producing VST3 (+AU for mac)
- Freeze and granular playback is audible and stable at low CPU settings
- Plugin loads in Ableton Live (macOS AU/VST3) and Bitwig (VST3)
- Installer or zip artifact is produced by CI for both platforms

Optional future features
- Multi-band freezing, spectral-granular mode, Ableton Link sync, AAX port

