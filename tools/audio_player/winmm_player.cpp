#define NOMINMAX
#include "audio/wav.h"
#include "dsp/morph_engine.h"
#include <Windows.h>
#include <conio.h>
#include <mmsystem.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
constexpr unsigned kBlockFrames=256;
struct Block {
    std::vector<float> samples=std::vector<float>(kBlockFrames*2);
    std::vector<std::int16_t> pcm=std::vector<std::int16_t>(kBlockFrames*2);
    WAVEHDR header{};
    bool queued=false;
};
std::int16_t to_pcm(float x) {
    x=std::clamp(std::isfinite(x)?x:0.0f,-1.0f,0.999969f);
    return static_cast<std::int16_t>(std::lrint(x*32768.0f));
}
void check(MMRESULT result,const char* operation) {
    if(result==MMSYSERR_NOERROR) return;
    char message[MAXERRORLENGTH]={};
    waveOutGetErrorTextA(result,message,MAXERRORLENGTH);
    throw std::runtime_error(std::string(operation)+": "+message);
}
}

int main(int argc,char** argv) try {
    if(argc!=2) {
        std::cerr<<"Usage: vortex-play <stereo-input.wav>\n";
        return 2;
    }
    const auto audio=vortex::read_wav(argv[1]);
    if(audio.channels!=2) throw std::runtime_error("stereo WAV input is required");
    if(audio.sample_rate!=44100 && audio.sample_rate!=48000)
        throw std::runtime_error("only 44.1 kHz and 48 kHz input is supported");

    WAVEFORMATEX format{};
    format.wFormatTag=WAVE_FORMAT_PCM;
    format.nChannels=2;
    format.nSamplesPerSec=audio.sample_rate;
    format.wBitsPerSample=16;
    format.nBlockAlign=4;
    format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;
    HWAVEOUT output=nullptr;
    check(waveOutOpen(&output,WAVE_MAPPER,&format,0,0,CALLBACK_NULL),"open audio output");

    vortex::MorphEngine engine;
    engine.prepare(audio.sample_rate,kBlockFrames);
    vortex::MorphParams params;
    params.wet=0.5f;
    std::array<Block,3> blocks;
    for(auto& block:blocks) {
        block.header.lpData=reinterpret_cast<LPSTR>(block.pcm.data());
        block.header.dwBufferLength=DWORD(block.pcm.size()*sizeof(std::int16_t));
        check(waveOutPrepareHeader(output,&block.header,sizeof(WAVEHDR)),"prepare playback buffer");
    }

    std::size_t source_frame=0;
    bool quitting=false;
    auto queue_block=[&](Block& block) {
        for(unsigned frame=0;frame<kBlockFrames;++frame) {
            const std::size_t source=source_frame*2;
            block.samples[2*frame]=audio.samples[source];
            block.samples[2*frame+1]=audio.samples[source+1];
            source_frame=(source_frame+1)%(audio.samples.size()/2);
        }
        engine.process(block.samples.data(),kBlockFrames,params);
        std::transform(block.samples.begin(),block.samples.end(),block.pcm.begin(),to_pcm);
        block.header.dwFlags&=~WHDR_DONE;
        check(waveOutWrite(output,&block.header,sizeof(WAVEHDR)),"queue playback buffer");
        block.queued=true;
    };
    for(auto& block:blocks) queue_block(block);

    std::cout<<"Playing loop. Morph: [/] or +/-; keys 0-9 set position; Q stops.\n"
             <<"Morph "<<params.morph<<"   ";
    while(!quitting) {
        for(auto& block:blocks) {
            if(block.queued && (block.header.dwFlags&WHDR_DONE)) {
                block.queued=false;
                queue_block(block);
            }
        }
        if(_kbhit()) {
            const int key=_getch();
            if(key=='q'||key=='Q') quitting=true;
            else if(key>='0'&&key<='9') params.morph=float(key-'0')/9.0f;
            else if(key=='+'||key==']') params.morph=std::min(1.0f,params.morph+0.01f);
            else if(key=='-'||key=='[') params.morph=std::max(0.0f,params.morph-0.01f);
            if(!quitting) std::cout<<"\rMorph "<<params.morph<<"   "<<std::flush;
        }
        Sleep(1);
    }
    waveOutReset(output);
    for(auto& block:blocks) waveOutUnprepareHeader(output,&block.header,sizeof(WAVEHDR));
    waveOutClose(output);
    std::cout<<"\nPlayback stopped.\n";
    return 0;
} catch(const std::exception& error) {
    std::cerr<<"vortex-play: "<<error.what()<<"\n";
    return 1;
}
