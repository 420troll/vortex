#pragma once
#include "audio/wav.h"
#include "dsp/morph_engine.h"
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace vortex {
enum class TransportState : std::uint8_t { Stopped, Playing, Paused };
enum class AudioCommandType : std::uint8_t { Play, Pause, Stop, SetLoop, SetMorph, SetMorphTime, SetWet, SetInputGain, SetOutputGain };
struct AudioCommand { AudioCommandType type{}; float value{}; };

// Single producer (UI) / single consumer (audio worker), fixed-capacity queue.
template<std::size_t Capacity>
class SpscCommandQueue {
    static_assert(Capacity > 1);
public:
    bool push(AudioCommand command) noexcept {
        const auto write=write_.load(std::memory_order_relaxed);
        const auto read=read_.load(std::memory_order_acquire);
        if(write-read==Capacity) return false;
        commands_[write%Capacity]=command;
        write_.store(write+1,std::memory_order_release);
        return true;
    }
    bool pop(AudioCommand& command) noexcept {
        const auto read=read_.load(std::memory_order_relaxed);
        const auto write=write_.load(std::memory_order_acquire);
        if(read==write) return false;
        command=commands_[read%Capacity];
        read_.store(read+1,std::memory_order_release);
        return true;
    }
    void clear() noexcept {
        read_.store(0,std::memory_order_relaxed);
        write_.store(0,std::memory_order_relaxed);
    }
private:
    std::array<AudioCommand,Capacity> commands_{};
    std::atomic<std::size_t> write_{0};
    std::atomic<std::size_t> read_{0};
};

class AudioSession {
public:
    static constexpr unsigned command_capacity=256;
    static constexpr unsigned max_block_frames=1024;
    // Call only while the device worker is stopped. Loading and allocation stay off the audio thread.
    void set_source(AudioBuffer audio);
    const AudioBuffer& source() const noexcept { return source_; }
    bool post(AudioCommand command) noexcept { return commands_.push(command); }
    void render(float* interleaved_stereo, unsigned frames) noexcept;
    TransportState state() const noexcept { return state_.load(std::memory_order_acquire); }
    std::size_t position_frames() const noexcept { return position_snapshot_.load(std::memory_order_relaxed); }
    std::size_t total_frames() const noexcept { return source_.samples.size()/2; }
    float morph_position() const noexcept { return morph_snapshot_.load(std::memory_order_relaxed); }
    float peak_left() const noexcept { return peak_left_.load(std::memory_order_relaxed); }
    float peak_right() const noexcept { return peak_right_.load(std::memory_order_relaxed); }
private:
    void apply_commands() noexcept;
    AudioBuffer source_;
    MorphEngine engine_;
    MorphParams params_{};
    float input_gain_=1.0f, output_gain_=1.0f;
    bool loop_=true;
    std::size_t position_=0;
    SpscCommandQueue<command_capacity> commands_;
    std::atomic<TransportState> state_{TransportState::Stopped};
    std::atomic<std::size_t> position_snapshot_{0};
    std::atomic<float> morph_snapshot_{0.5f};
    std::atomic<float> peak_left_{0.0f}, peak_right_{0.0f};
};

AudioBuffer render_offline(const AudioBuffer& source, const MorphParams& params,
                           float input_gain, float output_gain);
void validate_realtime_audio(const AudioBuffer& source);
}
