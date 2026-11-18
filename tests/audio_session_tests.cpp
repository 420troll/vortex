#include "audio/audio_session.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
vortex::AudioBuffer sample_audio(std::size_t frames) {
    vortex::AudioBuffer audio{48000,2,{}};
    audio.samples.resize(frames*2);
    for(std::size_t i=0;i<frames;++i) {
        const float value=0.2f*std::sin(float(i)*0.07f);
        audio.samples[2*i]=value; audio.samples[2*i+1]=-value;
    }
    return audio;
}
}

int main() try {
    vortex::SpscCommandQueue<2> queue;
    require(queue.push({vortex::AudioCommandType::SetMorph,0.2f}),"first bounded-queue push");
    require(queue.push({vortex::AudioCommandType::SetMorph,0.8f}),"second bounded-queue push");
    require(!queue.push({vortex::AudioCommandType::SetMorph,0.5f}),"bounded queue must report full");
    vortex::AudioCommand queued{};
    require(queue.pop(queued)&&std::abs(queued.value-0.2f)<1e-6f,"queue preserves command order");

    // Live target changes are consumed on the next render block and smoothed in the DSP.
    vortex::MorphEngine engine; engine.prepare(48000,512);
    vortex::MorphParams params; params.morph=1.0f; params.morph_time_ms=100.0f;
    std::vector<float> long_block(12000*2);
    engine.process(long_block.data(),480,params);
    require(engine.current_morph()>0.54f && engine.current_morph()<0.56f,"morph ramp should move gradually");
    engine.process(long_block.data(),12000,params);
    require(engine.current_morph()>0.95f,"morph ramp should approach its target");

    // Extreme feedback and input are finite and bounded by the engine's working range.
    engine.reset(); params.morph=0.5f; params.wet=1.0f; params.feedback_a=1.0f; params.feedback_b=1.0f;
    params.delay_a_ms=params.delay_b_ms=1.0f;
    float block[512*2]{};
    std::fill(std::begin(block),std::end(block),100.0f);
    for(int i=0;i<24;++i) engine.process(block,512,params);
    for(float value:block) require(std::isfinite(value)&&std::abs(value)<=4.0f,"DSP output bound or finite-value check failed");
    engine.reset(); std::fill(std::begin(block),std::end(block),0.0f); engine.process(block,512,params);
    for(float value:block) require(value==0.0f,"reset must invalidate delay state");

    // Transport commands are deterministic without an audio device.
    vortex::AudioSession session; session.set_source(sample_audio(4));
    require(session.post({vortex::AudioCommandType::SetLoop,0}),"queue SetLoop");
    require(session.post({vortex::AudioCommandType::Play,1}),"queue Play");
    float transport[12]{}; session.render(transport,2);
    require(session.state()==vortex::TransportState::Playing && session.position_frames()==2,"play advances cursor");
    const auto held_position=session.position_frames();
    require(session.post({vortex::AudioCommandType::Pause,0}),"queue Pause");
    session.render(transport,2);
    require(session.state()==vortex::TransportState::Paused && session.position_frames()==held_position,"pause holds cursor");
    require(session.post({vortex::AudioCommandType::Play,1}),"queue resume");
    session.render(transport,2);
    require(session.state()==vortex::TransportState::Stopped && session.position_frames()==4,"end of file stops transport");
    require(session.post({vortex::AudioCommandType::Play,1}),"queue replay"); session.render(transport,1);
    require(session.state()==vortex::TransportState::Playing && session.position_frames()==1,"replay restarts from beginning");
    require(session.post({vortex::AudioCommandType::Stop,0}),"queue Stop"); session.render(transport,1);
    require(session.state()==vortex::TransportState::Stopped && session.position_frames()==0,"stop resets cursor");
    require(session.post({vortex::AudioCommandType::SetLoop,1}),"queue loop");
    require(session.post({vortex::AudioCommandType::Play,1}),"queue looping Play"); session.render(transport,6);
    require(session.state()==vortex::TransportState::Playing && session.position_frames()==2,"loop wraps end of file");

    // GUI-style morph commands update while the device remains active.
    session.set_source(sample_audio(20000));
    require(session.post({vortex::AudioCommandType::SetLoop,1}),"queue live loop");
    require(session.post({vortex::AudioCommandType::Play,1}),"queue live Play"); session.render(block,256);
    const float before=session.morph_position();
    require(session.post({vortex::AudioCommandType::SetMorphTime,100.0f}),"queue transition time");
    require(session.post({vortex::AudioCommandType::SetMorph,1.0f}),"queue live morph"); session.render(block,256);
    require(session.state()==vortex::TransportState::Playing && session.morph_position()>before,"morph updates while transport runs");
    require(session.post({vortex::AudioCommandType::SetInputGain,2.0f}),"queue input gain");
    require(session.post({vortex::AudioCommandType::SetOutputGain,2.0f}),"queue output gain");
    std::fill(std::begin(block),std::end(block),0.0f); session.render(block,256);
    for(float value:block) require(std::isfinite(value)&&std::abs(value)<=1.0f,"session output must be bounded");

    vortex::AudioBuffer loud{48000,2,std::vector<float>(2048,10.0f)};
    vortex::AudioSession limiter; limiter.set_source(std::move(loud));
    limiter.post({vortex::AudioCommandType::SetWet,0.0f});
    limiter.post({vortex::AudioCommandType::SetInputGain,2.0f});
    limiter.post({vortex::AudioCommandType::SetOutputGain,2.0f});
    limiter.post({vortex::AudioCommandType::Play,1.0f}); limiter.render(block,256);
    for(unsigned i=0;i<256*2;++i) require(block[i]==1.0f,"session output must clip to the WAV/device range");
    require(limiter.peak_left()==1.0f&&limiter.peak_right()==1.0f,"stereo peak meters report clipped output");

    auto export_audio=vortex::render_offline(sample_audio(2048),params,1.0f,1.0f);
    vortex::write_wav("vortex-export-test.wav",export_audio);
    const auto loaded=vortex::read_wav("vortex-export-test.wav"); std::remove("vortex-export-test.wav");
    require(loaded.channels==2 && loaded.sample_rate==48000 && loaded.samples.size()==4096,"offline WAV export roundtrip");
    for(float value:loaded.samples) require(std::isfinite(value)&&std::abs(value)<=1.0f,"export sample range");
    bool missing_rejected=false;
    try { (void)vortex::read_wav("vortex-definitely-missing-test.wav"); }
    catch(const std::exception&) { missing_rejected=true; }
    require(missing_rejected,"missing WAV should fail cleanly");
    { std::ofstream bad("vortex-invalid-test.wav",std::ios::binary); bad<<"not a wave file"; }
    bool invalid_rejected=false;
    try { (void)vortex::read_wav("vortex-invalid-test.wav"); }
    catch(const std::exception&) { invalid_rejected=true; }
    std::remove("vortex-invalid-test.wav");
    require(invalid_rejected,"malformed WAV should fail cleanly");
    auto mono=sample_audio(16); mono.channels=1; mono.samples.resize(16);
    bool mono_rejected=false;
    try { vortex::validate_realtime_audio(mono); }
    catch(const std::exception&) { mono_rejected=true; }
    require(mono_rejected,"mono input should be rejected by real-time playback validation");
    auto unsupported_rate=sample_audio(16); unsupported_rate.sample_rate=96000;
    bool rate_rejected=false;
    try { vortex::validate_realtime_audio(unsupported_rate); }
    catch(const std::exception&) { rate_rejected=true; }
    require(rate_rejected,"unsupported playback sample rate should be rejected cleanly");
    std::cout<<"DSP smoothing, bounds, transport, live morph, queue and WAV export passed\n";
    return 0;
} catch(const std::exception& error) {
    std::cerr<<error.what()<<"\n"; std::remove("vortex-export-test.wav"); return 1;
}
