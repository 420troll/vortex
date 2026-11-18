# Native morph engine

The native engine is independent C++20 code and does not load the Lexicon ROM. Its initial scene uses a shared stereo delay buffer, per-sample smoothed morph position, fractional delay reads, interpolated delay time and feedback, and a cross-feedback path. This is a minimal Vortex-inspired experiment, not an authentic Vortex algorithm.

The DSP library has no UI or device dependencies. `AudioSession` receives commands from the UI through a bounded SPSC queue at render-block boundaries. Its render path avoids disk access, heap allocation, blocking locks, and GUI calls. The WinMM worker handles device setup, PCM buffers, error reporting, reset, and close.

Current limitations: no modulation, filtering, envelope follower, parameter persistence, or performance instrumentation. Feedback is clamped below unity and delay state is bounded to reduce runaway behavior. The GUI uses the default Windows output device; physical playback has not been tested.
