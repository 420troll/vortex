#pragma once
#ifdef _WIN32
#include "audio/audio_session.h"
#include <atomic>
#include <array>
#include <cstdint>
#include <thread>

namespace vortex {
class WinMmOutput {
public:
    explicit WinMmOutput(AudioSession& session) noexcept : session_(session) {}
    ~WinMmOutput();
    WinMmOutput(const WinMmOutput&)=delete;
    WinMmOutput& operator=(const WinMmOutput&)=delete;
    bool start() noexcept;
    void shutdown() noexcept;
    bool running() const noexcept { return ready_.load(std::memory_order_acquire)==1; }
    bool failed() const noexcept { return failed_.load(std::memory_order_acquire); }
    const char* error_message() const noexcept { return error_message_.data(); }
    std::uint32_t error_code() const noexcept { return error_code_.load(std::memory_order_relaxed); }
    unsigned queued_buffers() const noexcept { return queued_buffers_.load(std::memory_order_relaxed); }
    std::uint64_t underrun_count() const noexcept { return underruns_.load(std::memory_order_relaxed); }
    std::uint64_t last_render_microseconds() const noexcept { return last_render_us_.load(std::memory_order_relaxed); }
    std::uint64_t max_render_microseconds() const noexcept { return max_render_us_.load(std::memory_order_relaxed); }
    std::uint64_t submitted_frames() const noexcept { return submitted_frames_.load(std::memory_order_relaxed); }
    std::uint64_t completed_frames() const noexcept { return completed_frames_.load(std::memory_order_relaxed); }
    std::size_t played_position_frames() const noexcept { return played_position_.load(std::memory_order_relaxed); }
    std::uint32_t output_sample_rate() const noexcept;
    static constexpr unsigned buffer_frames() noexcept { return 1024; }
    static constexpr unsigned buffer_count() noexcept { return 3; }
private:
    void run() noexcept;
    void set_error(const char* operation,unsigned code) noexcept;
    AudioSession& session_;
    std::thread thread_;
    std::atomic<bool> stopping_{false}, failed_{false};
    std::atomic<int> ready_{0};
    std::atomic<void*> stop_event_{nullptr};
    std::atomic<std::uint32_t> error_code_{0},queued_buffers_{0};
    std::atomic<std::uint64_t> underruns_{0},last_render_us_{0},max_render_us_{0};
    std::atomic<std::uint64_t> submitted_frames_{0},completed_frames_{0};
    std::atomic<std::size_t> played_position_{0};
    std::array<char,256> error_message_{};
};
}
#endif
