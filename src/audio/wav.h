#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace vortex {
struct AudioBuffer {
    std::uint32_t sample_rate{};
    std::uint16_t channels{};
    std::vector<float> samples; // interleaved
};
AudioBuffer read_wav(const std::string& path);
void write_wav(const std::string& path, const AudioBuffer& audio);
}
