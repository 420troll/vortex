#define NOMINMAX
#include "audio/winmm_output.h"
#ifdef _WIN32
#include <Windows.h>
#include <mmsystem.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace vortex {
namespace {
constexpr unsigned kFrames=256;
struct Block { std::array<float,kFrames*2> float_samples{}; std::array<std::int16_t,kFrames*2> pcm{}; WAVEHDR header{}; bool queued=false; };
std::int16_t quantize(float value) noexcept {
    value=std::clamp(std::isfinite(value)?value:0.0f,-1.0f,0.999969f);
    return static_cast<std::int16_t>(std::lrint(value*32768.0f));
}
}

WinMmOutput::~WinMmOutput() { shutdown(); }

bool WinMmOutput::start() noexcept {
    if(thread_.joinable()) return ready_.load(std::memory_order_acquire)==1;
    const auto rate=session_.source().sample_rate;
    if(rate!=44100 && rate!=48000) {
        std::snprintf(error_message_.data(),error_message_.size(),"unsupported audio sample rate: %u",rate);
        failed_.store(true,std::memory_order_release); return false;
    }
    stopping_.store(false,std::memory_order_relaxed);
    failed_.store(false,std::memory_order_relaxed);
    ready_.store(0,std::memory_order_relaxed);
    error_message_.fill('\0');
    try { thread_=std::thread(&WinMmOutput::run,this); }
    catch(...) {
        std::snprintf(error_message_.data(),error_message_.size(),"could not start audio worker thread");
        failed_.store(true,std::memory_order_release); ready_.store(-1,std::memory_order_release); return false;
    }
    for(unsigned ms=0;ms<10000;++ms) {
        const int ready=ready_.load(std::memory_order_acquire);
        if(ready==1) return true;
        if(ready==-1) { thread_.join(); return false; }
        Sleep(1);
    }
    set_error("audio device initialization timed out",0);
    shutdown();
    return false;
}

void WinMmOutput::shutdown() noexcept {
    if(!thread_.joinable()) return;
    stopping_.store(true,std::memory_order_release);
    thread_.join();
    ready_.store(0,std::memory_order_release);
}

void WinMmOutput::set_error(const char* operation,unsigned code) noexcept {
    char description[MAXERRORLENGTH]{};
    waveOutGetErrorTextA(static_cast<MMRESULT>(code),description,MAXERRORLENGTH);
    std::snprintf(error_message_.data(),error_message_.size(),"%s: %s (%u)",operation,description,code);
    failed_.store(true,std::memory_order_release);
}

void WinMmOutput::run() noexcept {
    const auto rate=session_.source().sample_rate;
    WAVEFORMATEX format{};
    format.wFormatTag=WAVE_FORMAT_PCM; format.nChannels=2; format.nSamplesPerSec=rate;
    format.wBitsPerSample=16; format.nBlockAlign=4; format.nAvgBytesPerSec=rate*4;
    HWAVEOUT output=nullptr;
    auto result=waveOutOpen(&output,WAVE_MAPPER,&format,0,0,CALLBACK_NULL);
    if(result!=MMSYSERR_NOERROR) { set_error("opening Windows audio output",result); ready_.store(-1,std::memory_order_release); return; }
    std::array<Block,3> blocks{};
    unsigned prepared=0;
    for(auto& block:blocks) {
        block.header.lpData=reinterpret_cast<LPSTR>(block.pcm.data());
        block.header.dwBufferLength=DWORD(block.pcm.size()*sizeof(std::int16_t));
        result=waveOutPrepareHeader(output,&block.header,sizeof(WAVEHDR));
        if(result!=MMSYSERR_NOERROR) { set_error("preparing Windows audio buffer",result); break; }
        ++prepared;
    }
    if(prepared!=blocks.size()) {
        for(unsigned i=0;i<prepared;++i) waveOutUnprepareHeader(output,&blocks[i].header,sizeof(WAVEHDR));
        waveOutClose(output); ready_.store(-1,std::memory_order_release); return;
    }
    auto submit=[&](Block& block) {
        session_.render(block.float_samples.data(),kFrames);
        std::transform(block.float_samples.begin(),block.float_samples.end(),block.pcm.begin(),quantize);
        block.header.dwFlags&=~WHDR_DONE;
        const auto status=waveOutWrite(output,&block.header,sizeof(WAVEHDR));
        if(status!=MMSYSERR_NOERROR) { set_error("submitting Windows audio buffer",status); return false; }
        block.queued=true; return true;
    };
    bool healthy=true;
    for(auto& block:blocks) if(!submit(block)) { healthy=false; break; }
    if(!healthy) {
        waveOutReset(output); for(auto& block:blocks) if(block.header.dwFlags&WHDR_PREPARED) waveOutUnprepareHeader(output,&block.header,sizeof(WAVEHDR));
        waveOutClose(output); ready_.store(-1,std::memory_order_release); return;
    }
    ready_.store(1,std::memory_order_release);
    while(!stopping_.load(std::memory_order_acquire) && healthy) {
        for(auto& block:blocks) if(block.queued && (block.header.dwFlags&WHDR_DONE)) {
            block.queued=false;
            if(!submit(block)) { healthy=false; break; }
        }
        Sleep(1);
    }
    waveOutReset(output);
    for(auto& block:blocks) if(block.header.dwFlags&WHDR_PREPARED) waveOutUnprepareHeader(output,&block.header,sizeof(WAVEHDR));
    const auto close_result=waveOutClose(output);
    if(close_result!=MMSYSERR_NOERROR && !failed_.load(std::memory_order_relaxed)) set_error("closing Windows audio output",close_result);
    if(failed_.load(std::memory_order_acquire)) ready_.store(-1,std::memory_order_release);
}
}
#endif
