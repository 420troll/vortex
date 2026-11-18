# Native morph engine

The native engine is independent C++20 code and does not load the Lexicon ROM. Its initial scene uses a shared stereo delay buffer, per-sample smoothed morph position, fractional delay reads, interpolated delay time and feedback, and a cross-feedback path. This is a minimal Vortex-inspired experiment, not an authentic Vortex algorithm.

Current limitations: no modulation, filtering, envelope follower, parameter persistence, or performance instrumentation. Feedback is clamped below unity and delay state is bounded to reduce runaway behavior. The Windows proof-of-concept player is a separate WinMM host; the DSP library has no user-interface or device dependencies.
