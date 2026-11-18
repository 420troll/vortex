#pragma once
#ifdef _WIN32
#include "audio/audio_session.h"
#include <atomic>
#include <array>
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
private:
    void run() noexcept;
    void set_error(const char* operation,unsigned code) noexcept;
    AudioSession& session_;
    std::thread thread_;
    std::atomic<bool> stopping_{false}, failed_{false};
    std::atomic<int> ready_{0};
    std::array<char,256> error_message_{};
};
}
#endif
