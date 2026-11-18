#pragma once
#include <cstdint>
#include <vector>
namespace vortex {
struct MorphParams { float morph=0.5f, morph_time_ms=25.0f, wet=0.35f, feedback_a=0.3f, feedback_b=0.65f, delay_a_ms=120.0f, delay_b_ms=480.0f; };
class MorphEngine {
public:
    void prepare(unsigned sample_rate, unsigned max_block);
    void reset() noexcept;
    void process(float* interleaved_stereo, unsigned frames, const MorphParams& params);
    float current_morph() const noexcept { return morph_current_; }
private:
    unsigned rate_=48000, cursor_=0;
    std::uint32_t generation_=1;
    float morph_current_=0.5f;
    std::vector<float> left_, right_;
    std::vector<std::uint32_t> slot_generation_;
};
}
