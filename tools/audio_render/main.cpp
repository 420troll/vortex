#include "audio/wav.h"
#include "dsp/morph_engine.h"
#include <cstdlib>
#include <iostream>
#include <string>
int main(int argc,char** argv) try {
 if(argc<3) { std::cerr<<"usage: vortex-render input.wav output.wav [morph 0..1] [wet 0..1]\n"; return 2; }
 auto audio=vortex::read_wav(argv[1]); if(audio.channels!=2) throw std::runtime_error("stereo WAV required");
 vortex::MorphEngine engine; engine.prepare(audio.sample_rate,512); vortex::MorphParams p;
 if(argc>3) p.morph=std::stof(argv[3]);
 if(argc>4) p.wet=std::stof(argv[4]);
 engine.process(audio.samples.data(),unsigned(audio.samples.size()/2),p); vortex::write_wav(argv[2],audio); return 0;
} catch(const std::exception& e) { std::cerr<<"vortex-render: "<<e.what()<<"\n"; return 1; }
