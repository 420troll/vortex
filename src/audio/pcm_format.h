#pragma once
#include <cstdint>

namespace vortex {
struct Pcm16StereoFormat {
    std::uint32_t sample_rate{};
    std::uint16_t channels=2;
    std::uint16_t bits_per_sample=16;
    std::uint16_t block_align=4;
    std::uint32_t average_bytes_per_second{};
};

constexpr Pcm16StereoFormat pcm16_stereo_format(std::uint32_t sample_rate) noexcept {
    return {sample_rate,2,16,4,sample_rate*4u};
}
constexpr std::uint32_t pcm16_stereo_buffer_bytes(std::uint32_t frames) noexcept {
    return frames*4u;
}
}
