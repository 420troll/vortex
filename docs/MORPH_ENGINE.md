# Native morph engine

The native engine is independent C++20 code and does not load the Lexicon ROM. Its initial scene uses a shared stereo delay buffer, interpolated delay time and feedback, and a cross-feedback path. This is a minimal Vortex-inspired experiment, not an authentic Vortex algorithm.

Current limitations: offline rendering only; integer-sample delay changes can shift abruptly during a render; no modulation, filtering, envelope follower, UI, device audio, parameter smoothing, project persistence, or performance instrumentation. Feedback is clamped below unity and delay state is bounded to reduce runaway behavior. Later work should use fractional read heads with dual-head crossfades for moving delay times and ramp parameter snapshots at block boundaries.
