#include "audio/audio_session.h"
#include "audio/pcm_format.h"
#include "audio/winmm_output.h"
#include <Windows.h>
#include <mmsystem.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

namespace {
bool run_single_buffer_probe(std::uint32_t rate) {
    const auto pcm_format=vortex::pcm16_stereo_format(rate);
    WAVEFORMATEX format{}; format.wFormatTag=WAVE_FORMAT_PCM; format.nChannels=pcm_format.channels;
    format.nSamplesPerSec=pcm_format.sample_rate; format.wBitsPerSample=pcm_format.bits_per_sample;
    format.nBlockAlign=pcm_format.block_align; format.nAvgBytesPerSec=pcm_format.average_bytes_per_second;
    HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr); if(!event) return false;
    HWAVEOUT output=nullptr;
    auto result=waveOutOpen(&output,WAVE_MAPPER,&format,reinterpret_cast<DWORD_PTR>(event),0,CALLBACK_EVENT);
    if(result!=MMSYSERR_NOERROR) { CloseHandle(event); return false; }
    std::vector<std::int16_t> data(std::size_t(rate)*2);
    for(std::uint32_t frame=0;frame<rate;++frame) {
        const auto value=static_cast<std::int16_t>(std::lrint(1600.0*std::sin(6.28318530718*440.0*double(frame)/double(rate))));
        data[2*frame]=value; data[2*frame+1]=value;
    }
    WAVEHDR header{}; header.lpData=reinterpret_cast<LPSTR>(data.data());
    header.dwBufferLength=DWORD(data.size()*sizeof(std::int16_t));
    result=waveOutPrepareHeader(output,&header,sizeof(header));
    if(result==MMSYSERR_NOERROR) result=waveOutWrite(output,&header,sizeof(header));
    const auto begin=std::chrono::steady_clock::now();
    const auto deadline=begin+std::chrono::seconds(5);
    while(result==MMSYSERR_NOERROR && !(header.dwFlags&WHDR_DONE) && std::chrono::steady_clock::now()<deadline)
        WaitForSingleObject(event,1000);
    const auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
    const bool done=(header.dwFlags&WHDR_DONE)!=0;
    std::cout<<"Single-buffer WinMM baseline at "<<rate<<" Hz: done="<<done<<", elapsed="<<elapsed<<" s\n";
    waveOutReset(output); if(header.dwFlags&WHDR_PREPARED) waveOutUnprepareHeader(output,&header,sizeof(header));
    waveOutClose(output); CloseHandle(event);
    return done && std::abs(elapsed-1.0)<0.15;
}

bool run_probe(std::uint32_t rate) {
    vortex::AudioBuffer source{rate,2,{}};
    source.samples.resize(std::size_t(rate)*2);
    for(std::uint32_t frame=0;frame<rate;++frame) {
        const float sample=0.05f*std::sin(6.28318530718f*440.0f*float(frame)/float(rate));
        source.samples[2*frame]=sample; source.samples[2*frame+1]=sample;
    }
    vortex::AudioSession session; session.set_source(std::move(source));
    session.post({vortex::AudioCommandType::SetLoop,0.0f});
    session.post({vortex::AudioCommandType::SetBypass,1.0f});
    session.post({vortex::AudioCommandType::Play,1.0f});
    vortex::WinMmOutput output(session);
    if(!output.start()) {
        std::cerr<<"WinMM open failed at "<<rate<<" Hz: "<<output.error_message()<<"\n";
        return false;
    }
    auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
    while(output.completed_frames()==0 && !output.failed() && std::chrono::steady_clock::now()<deadline) Sleep(1);
    if(output.failed() || output.completed_frames()==0) {
        std::cerr<<"No WinMM buffer completion at "<<rate<<" Hz: "<<output.error_message()<<"\n";
        output.shutdown(); return false;
    }
    const auto baseline=output.completed_frames();
    const auto begin=std::chrono::steady_clock::now();
    const auto target=baseline+rate;
    deadline=begin+std::chrono::seconds(5);
    while(output.completed_frames()<target && !output.failed() && std::chrono::steady_clock::now()<deadline) Sleep(1);
    const auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
    const bool completed=output.completed_frames()>=target;
    std::cout<<rate<<" Hz stereo PCM16; block="<<vortex::WinMmOutput::buffer_frames()<<" frames / "
             <<vortex::pcm16_stereo_buffer_bytes(vortex::WinMmOutput::buffer_frames())<<" bytes; measured="
             <<elapsed<<" s for "<<rate<<" completed frames; submitted="<<output.submitted_frames()
             <<" frames, queued="<<output.queued_buffers()<<", underruns="<<output.underrun_count()
             <<"; max render="<<output.max_render_microseconds()<<" us\n";
    const bool passed=completed && !output.failed() && output.underrun_count()==0 && std::abs(elapsed-1.0)<0.08;
    session.post({vortex::AudioCommandType::Stop,0.0f}); output.shutdown();
    if(!passed) std::cerr<<"WinMM timing check failed at "<<rate<<" Hz\n";
    return passed;
}
}
int main() {
    WAVEOUTCAPSW caps{};
    if(waveOutGetDevCapsW(WAVE_MAPPER,&caps,sizeof(caps))==MMSYSERR_NOERROR)
        std::wcout<<L"Default WinMM mapper: "<<caps.szPname<<L" ("<<caps.wChannels<<L" channels)\n";
    const bool direct_441=run_single_buffer_probe(44100);
    const bool direct_480=run_single_buffer_probe(48000);
    const bool queued_441=run_probe(44100);
    const bool queued_480=run_probe(48000);
    return direct_441&&direct_480&&queued_441&&queued_480?0:1;
}
