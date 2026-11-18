#include "audio/audio_session.h"
#include "audio/winmm_output.h"
#define NOMINMAX
#include <Windows.h>
#include <conio.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>

int main(int argc,char** argv) try {
    if(argc!=2) {
        std::cerr<<"Usage: vortex-play <stereo-input.wav>\n";
        return 2;
    }
    vortex::AudioSession session;
    session.set_source(vortex::read_wav(argv[1]));
    vortex::WinMmOutput output(session);
    if(!session.post({vortex::AudioCommandType::Play,1.0f}) || !output.start())
        throw std::runtime_error(output.error_message());

    float morph=0.5f;
    std::cout<<"Looping. Morph [/] or +/-; keys 0-9 set position; P pauses/resumes; S stops; Q exits.\n";
    while(true) {
        if(output.failed()) throw std::runtime_error(output.error_message());
        if(_kbhit()) {
            const int key=_getch();
            if(key=='q'||key=='Q') break;
            bool ok=true;
            if(key>='0'&&key<='9') morph=float(key-'0')/9.0f;
            else if(key=='+'||key==']') morph=std::min(1.0f,morph+0.01f);
            else if(key=='-'||key=='[') morph=std::max(0.0f,morph-0.01f);
            else if(key==' '||key=='p'||key=='P') {
                const auto type=session.state()==vortex::TransportState::Playing?vortex::AudioCommandType::Pause:vortex::AudioCommandType::Play;
                ok=session.post({type,1.0f});
            }
            if((key>='0'&&key<='9')||key=='+'||key==']'||key=='-'||key=='[')
                ok=session.post({vortex::AudioCommandType::SetMorph,morph});
            else if(key=='s'||key=='S') ok=session.post({vortex::AudioCommandType::Stop,0.0f});
            if(!ok) std::cerr<<"Audio command queue full; change was not applied.\n";
            std::cout<<"State: "<<int(session.state())<<"  Morph: "<<morph<<"\n";
        }
        Sleep(5);
    }
    session.post({vortex::AudioCommandType::Stop,0.0f});
    output.shutdown();
    return 0;
} catch(const std::exception& error) {
    std::cerr<<"vortex-play: "<<error.what()<<"\n";
    return 1;
}
