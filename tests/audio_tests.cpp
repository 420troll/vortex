#include "audio/wav.h"
#include "dsp/morph_engine.h"
#include <cmath>
#include <cstdio>
#include <iostream>
int main() {
 try {
  vortex::AudioBuffer a{48000,2,{0.25f,-0.25f,0.5f,-0.5f}};
  vortex::write_wav("vortex-test.wav",a); auto b=vortex::read_wav("vortex-test.wav"); std::remove("vortex-test.wav");
  if(b.sample_rate!=a.sample_rate || b.channels!=2 || b.samples.size()!=4 || std::abs(b.samples[0]-a.samples[0])>1.0f/32768) return 1;
  vortex::MorphEngine e; e.prepare(48000,64); float impulse[128]{}; impulse[0]=impulse[1]=1; vortex::MorphParams p; p.wet=0.5f; e.process(impulse,64,p);
  for(float x:impulse) if(!std::isfinite(x)) return 2;
  if(std::abs(impulse[0]-0.5f)>1e-6) return 3;
  std::cout<<"WAV roundtrip and morph processing passed\n"; return 0;
 } catch(const std::exception& e) { std::cerr<<e.what()<<"\n"; return 4; }
}
