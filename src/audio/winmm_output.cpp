#define NOMINMAX
#include "audio/winmm_output.h"
#ifdef _WIN32
#include "audio/buffer_ownership.h"
#include "audio/pcm_format.h"
#include <Windows.h>
#include <mmsystem.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>

namespace vortex {
namespace {
constexpr unsigned kFrames=WinMmOutput::buffer_frames();
constexpr unsigned kBufferCount=WinMmOutput::buffer_count();
struct Block {
    std::array<float,kFrames*2> float_samples{};
    std::array<std::int16_t,kFrames*2> pcm{};
    WAVEHDR header{};
    std::size_t source_position_end=0;
};
std::int16_t quantize(float value) noexcept {
    value=std::clamp(std::isfinite(value)?value:0.0f,-1.0f,0.999969f);
    return static_cast<std::int16_t>(std::lrint(value*32768.0f));
}
}

WinMmOutput::~WinMmOutput() { shutdown(); }

std::uint32_t WinMmOutput::output_sample_rate() const noexcept { return session_.source().sample_rate; }

bool WinMmOutput::start() noexcept {
    if(thread_.joinable()) {
        if(ready_.load(std::memory_order_acquire)==1) return true;
        shutdown(); // Join and release a worker that previously exited after a device error.
    }
    const auto rate=output_sample_rate();
    if(rate!=44100 && rate!=48000) {
        std::snprintf(error_message_.data(),error_message_.size(),"unsupported output sample rate: %u",rate);
        error_code_.store(0,std::memory_order_relaxed);
        failed_.store(true,std::memory_order_release); return false;
    }
    HANDLE stop_event=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    if(!stop_event) {
        const auto code=GetLastError();
        std::snprintf(error_message_.data(),error_message_.size(),"creating audio stop event failed (%lu)",code);
        error_code_.store(code,std::memory_order_relaxed);
        failed_.store(true,std::memory_order_release); return false;
    }
    stop_event_.store(stop_event,std::memory_order_release);
    stopping_.store(false,std::memory_order_relaxed);
    failed_.store(false,std::memory_order_relaxed);
    ready_.store(0,std::memory_order_relaxed);
    error_code_.store(0,std::memory_order_relaxed);
    error_message_.fill('\0');
    underruns_.store(0,std::memory_order_relaxed); last_render_us_.store(0,std::memory_order_relaxed);
    max_render_us_.store(0,std::memory_order_relaxed); submitted_frames_.store(0,std::memory_order_relaxed);
    completed_frames_.store(0,std::memory_order_relaxed); played_position_.store(0,std::memory_order_relaxed);
    queued_buffers_.store(0,std::memory_order_relaxed);
    try { thread_=std::thread(&WinMmOutput::run,this); }
    catch(...) {
        CloseHandle(stop_event); stop_event_.store(nullptr,std::memory_order_release);
        std::snprintf(error_message_.data(),error_message_.size(),"could not start audio worker thread");
        failed_.store(true,std::memory_order_release); ready_.store(-1,std::memory_order_release); return false;
    }
    for(unsigned ms=0;ms<10000;++ms) {
        const int ready=ready_.load(std::memory_order_acquire);
        if(ready==1) return true;
        if(ready==-1) { thread_.join(); CloseHandle(stop_event); stop_event_.store(nullptr,std::memory_order_release); return false; }
        Sleep(1); // Startup handshake only; playback scheduling is event-driven.
    }
    set_error("audio device initialization timed out",0);
    shutdown();
    return false;
}

void WinMmOutput::shutdown() noexcept {
    if(!thread_.joinable()) return;
    stopping_.store(true,std::memory_order_release);
    HANDLE stop_event=static_cast<HANDLE>(stop_event_.load(std::memory_order_acquire));
    if(stop_event) SetEvent(stop_event);
    thread_.join();
    ready_.store(0,std::memory_order_release);
    if(stop_event) CloseHandle(stop_event);
    stop_event_.store(nullptr,std::memory_order_release);
}

void WinMmOutput::set_error(const char* operation,unsigned code) noexcept {
    char description[MAXERRORLENGTH]{};
    if(code) waveOutGetErrorTextA(static_cast<MMRESULT>(code),description,MAXERRORLENGTH);
    std::snprintf(error_message_.data(),error_message_.size(),"%s: %s (%u)",operation,description,code);
    error_code_.store(code,std::memory_order_relaxed);
    failed_.store(true,std::memory_order_release);
}

void WinMmOutput::run() noexcept {
    const auto pcm=pcm16_stereo_format(session_.source().sample_rate);
    WAVEFORMATEX format{};
    format.wFormatTag=WAVE_FORMAT_PCM;
    format.nChannels=pcm.channels;
    format.nSamplesPerSec=pcm.sample_rate;
    format.wBitsPerSample=pcm.bits_per_sample;
    format.nBlockAlign=pcm.block_align;
    format.nAvgBytesPerSec=pcm.average_bytes_per_second;

    HANDLE completion_event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    if(!completion_event) {
        set_error("creating WinMM completion event failed",GetLastError());
        ready_.store(-1,std::memory_order_release); return;
    }
    HANDLE stop_event=static_cast<HANDLE>(stop_event_.load(std::memory_order_acquire));
    HWAVEOUT output=nullptr;
    auto result=waveOutOpen(&output,WAVE_MAPPER,&format,reinterpret_cast<DWORD_PTR>(completion_event),0,CALLBACK_EVENT);
    if(result!=MMSYSERR_NOERROR) {
        set_error("opening Windows audio output",result); CloseHandle(completion_event);
        ready_.store(-1,std::memory_order_release); return;
    }

    std::array<Block,kBufferCount> blocks{};
    unsigned prepared=0;
    for(auto& block:blocks) {
        block.header.lpData=reinterpret_cast<LPSTR>(block.pcm.data());
        block.header.dwBufferLength=pcm16_stereo_buffer_bytes(kFrames);
        result=waveOutPrepareHeader(output,&block.header,sizeof(WAVEHDR));
        if(result!=MMSYSERR_NOERROR) { set_error("preparing Windows audio buffer",result); break; }
        ++prepared;
    }
    if(prepared!=blocks.size()) {
        for(unsigned i=0;i<prepared;++i) waveOutUnprepareHeader(output,&blocks[i].header,sizeof(WAVEHDR));
        waveOutClose(output); CloseHandle(completion_event); ready_.store(-1,std::memory_order_release); return;
    }

    BufferOwnership<kBufferCount> ownership;
    auto submit=[&](unsigned index) {
        if(index>=blocks.size() || ownership.is_queued(index)) return false;
        auto& block=blocks[index];
        const auto begin=std::chrono::steady_clock::now();
        session_.render(block.float_samples.data(),kFrames);
        std::transform(block.float_samples.begin(),block.float_samples.end(),block.pcm.begin(),quantize);
        const auto elapsed=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-begin).count();
        const auto render_us=static_cast<std::uint64_t>(std::max<std::int64_t>(0,elapsed));
        last_render_us_.store(render_us,std::memory_order_relaxed);
        auto maximum=max_render_us_.load(std::memory_order_relaxed);
        while(render_us>maximum && !max_render_us_.compare_exchange_weak(maximum,render_us,std::memory_order_relaxed)) {}
        block.source_position_end=session_.position_frames();
        const auto status=waveOutWrite(output,&block.header,sizeof(WAVEHDR));
        if(status!=MMSYSERR_NOERROR) { set_error("submitting Windows audio buffer",status); return false; }
        if(!ownership.submit(index)) { set_error("WinMM buffer ownership invariant failed",0); return false; }
        submitted_frames_.fetch_add(kFrames,std::memory_order_relaxed);
        queued_buffers_.store(static_cast<unsigned>(ownership.queued_count()),std::memory_order_relaxed);
        return true;
    };

    bool healthy=true;
    for(unsigned i=0;i<kBufferCount;++i) if(!submit(i)) { healthy=false; break; }
    if(!healthy) {
        waveOutReset(output);
        for(auto& block:blocks) if(block.header.dwFlags&WHDR_PREPARED) waveOutUnprepareHeader(output,&block.header,sizeof(WAVEHDR));
        waveOutClose(output); CloseHandle(completion_event); queued_buffers_.store(0,std::memory_order_relaxed);
        ready_.store(-1,std::memory_order_release); return;
    }
    ready_.store(1,std::memory_order_release);

    HANDLE waits[2]{completion_event,stop_event};
    while(healthy && !stopping_.load(std::memory_order_acquire)) {
        const auto wait=WaitForMultipleObjects(2,waits,FALSE,INFINITE);
        if(wait==WAIT_OBJECT_0+1 || stopping_.load(std::memory_order_acquire)) break;
        if(wait!=WAIT_OBJECT_0) { set_error("waiting for WinMM buffer completion failed",GetLastError()); break; }

        unsigned completed_now=0;
        for(unsigned i=0;i<kBufferCount;++i) {
            auto& block=blocks[i];
            if(ownership.is_queued(i) && (block.header.dwFlags&WHDR_DONE)) {
                if(ownership.complete(i)) {
                    ++completed_now;
                    completed_frames_.fetch_add(kFrames,std::memory_order_relaxed);
                    played_position_.store(block.source_position_end,std::memory_order_relaxed);
                }
            }
        }
        queued_buffers_.store(static_cast<unsigned>(ownership.queued_count()),std::memory_order_relaxed);
        if(completed_now && ownership.queued_count()==0) underruns_.fetch_add(1,std::memory_order_relaxed);
        for(unsigned i=0;i<kBufferCount;++i) if(!ownership.is_queued(i) && !submit(i)) { healthy=false; break; }
    }

    waveOutReset(output);
    for(auto& block:blocks) if(block.header.dwFlags&WHDR_PREPARED) waveOutUnprepareHeader(output,&block.header,sizeof(WAVEHDR));
    const auto close_result=waveOutClose(output);
    if(close_result!=MMSYSERR_NOERROR && !failed_.load(std::memory_order_relaxed)) set_error("closing Windows audio output",close_result);
    CloseHandle(completion_event);
    queued_buffers_.store(0,std::memory_order_relaxed);
    if(failed_.load(std::memory_order_acquire)) ready_.store(-1,std::memory_order_release);
}
}
#endif
