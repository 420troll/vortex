#pragma once
#include <vector>
namespace vortex {
struct MorphParams { float morph=0.5f, wet=0.35f, feedback_a=0.3f, feedback_b=0.65f, delay_a_ms=120.0f, delay_b_ms=480.0f; };
class MorphEngine {
public:
    void prepare(unsigned sample_rate, unsigned max_block);
    void process(float* interleaved_stereo, unsigned frames, const MorphParams& params);
private:
    unsigned rate_=48000, cursor_=0;
    std::vector<float> left_, right_;
};
}
