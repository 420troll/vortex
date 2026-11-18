#include "audio/audio_session.h"
#include "audio/buffer_ownership.h"
#include "audio/pcm_format.h"
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
void verify_one_second_passthrough(std::uint32_t rate) {
    vortex::AudioBuffer generated{rate,2,{}};
    generated.samples.resize(std::size_t(rate)*2);
    for(std::uint32_t frame=0;frame<rate;++frame) {
        const float value=0.6f*std::sin(6.28318530718f*440.0f*float(frame)/float(rate));
        generated.samples[2*frame]=value; generated.samples[2*frame+1]=-value;
    }
    const auto path=rate==44100?"vortex-one-second-44100.wav":"vortex-one-second-48000.wav";
    vortex::write_wav(path,generated);
    auto source=vortex::read_wav(path); std::remove(path);
    require(source.sample_rate==rate && source.channels==2 && source.samples.size()==std::size_t(rate)*2,"one-second stereo WAV metadata and frame count");
    vortex::AudioSession session; session.set_source(std::move(source));
    require(session.post({vortex::AudioCommandType::SetLoop,0.0f}),"queue non-looping baseline");
    require(session.post({vortex::AudioCommandType::SetBypass,1.0f}),"queue DSP bypass");
    require(session.post({vortex::AudioCommandType::Play,1.0f}),"queue baseline Play");
    std::array<float,256*2> output{};
    std::size_t counted_frames=0,previous=0;
    unsigned blocks=0;
    while(session.state()!=vortex::TransportState::Stopped || session.position_frames()<session.total_frames()) {
        const auto start=session.position_frames();
        session.render(output.data(),256);
        const auto end=session.position_frames();
        require(end>=start,"non-looping frame cursor must not wrap");
        const auto active=end-start;
        for(std::size_t i=0;i<active;++i) {
            const auto frame=start+i;
            require(std::abs(output[i*2]-session.source().samples[frame*2])<1e-7f,"bypass left channel changed");
            require(std::abs(output[i*2+1]-session.source().samples[frame*2+1])<1e-7f,"bypass right channel changed");
        }
        counted_frames+=active; previous=end;
        require(++blocks<rate/128+4,"one-second baseline did not reach EOF");
    }
    (void)previous;
    require(counted_frames==rate,"one-second baseline frame count");
    const double duration=double(counted_frames)/double(rate);
    require(std::abs(duration-1.0)<1e-12,"one-second WAV duration calculation");
    require(session.bypassed(),"DSP bypass state should be active");
}
}

int main() try {
    constexpr auto format_441=vortex::pcm16_stereo_format(44100);
    constexpr auto format_480=vortex::pcm16_stereo_format(48000);
    require(format_441.channels==2 && format_441.bits_per_sample==16 && format_441.block_align==4 && format_441.average_bytes_per_second==176400,"44.1 kHz WinMM PCM format math");
    require(format_480.channels==2 && format_480.bits_per_sample==16 && format_480.block_align==4 && format_480.average_bytes_per_second==192000,"48 kHz WinMM PCM format math");
    require(vortex::pcm16_stereo_buffer_bytes(256)==1024,"256 stereo frames must occupy 1,024 bytes");
    require(vortex::pcm16_stereo_buffer_bytes(1024)==4096,"1,024 stereo frames must occupy 4,096 bytes");
    verify_one_second_passthrough(44100);
    verify_one_second_passthrough(48000);

    vortex::BufferOwnership<3> owners;
    require(owners.queued_count()==0 && owners.submit(0) && owners.submit(1) && owners.submit(2),"initial multi-buffer queue");
    require(owners.queued_count()==3 && !owners.submit(1),"WinMM-owned buffers cannot be overwritten");
    require(owners.complete(1) && owners.queued_count()==2 && !owners.complete(1),"only completed buffer becomes reusable");
    require(owners.submit(1) && owners.queued_count()==3,"completed buffer can be requeued");

    vortex::AudioSession loop_baseline;
    loop_baseline.set_source({48000,2,{0.25f,-0.25f,0.5f,-0.5f}});
    loop_baseline.post({vortex::AudioCommandType::SetBypass,1.0f});
    loop_baseline.post({vortex::AudioCommandType::SetLoop,1.0f});
    loop_baseline.post({vortex::AudioCommandType::Play,1.0f});
    float loop_output[10]{}; loop_baseline.render(loop_output,5);
    const float expected_loop[10]{0.25f,-0.25f,0.5f,-0.5f,0.25f,-0.25f,0.5f,-0.5f,0.25f,-0.25f};
    for(unsigned i=0;i<10;++i) require(loop_output[i]==expected_loop[i],"bypass loop boundary must neither repeat nor skip samples");
    require(loop_baseline.position_frames()==1,"looping file cursor wraps in frames");

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
