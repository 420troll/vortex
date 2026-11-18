#include "audio/audio_session.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace vortex {
void validate_realtime_audio(const AudioBuffer& source) {
    if(source.channels!=2 || source.samples.empty() || source.samples.size()%2)
        throw std::invalid_argument("a non-empty stereo WAV file is required");
    if(source.sample_rate!=44100 && source.sample_rate!=48000)
        throw std::invalid_argument("real-time playback supports 44.1 or 48 kHz WAV files");
}

void AudioSession::set_source(AudioBuffer audio) {
    if(audio.channels!=2 || audio.sample_rate<8000 || audio.sample_rate>384000 || audio.samples.empty() || audio.samples.size()%2)
        throw std::invalid_argument("audio session requires valid stereo audio");
    source_=std::move(audio);
    engine_.prepare(source_.sample_rate,max_block_frames);
    params_=MorphParams{};
    input_gain_=output_gain_=1.0f;
    loop_=true;
    position_=0;
    commands_.clear();
    position_snapshot_.store(0,std::memory_order_relaxed);
    morph_snapshot_.store(0.5f,std::memory_order_relaxed);
    peak_left_.store(0,std::memory_order_relaxed);
    peak_right_.store(0,std::memory_order_relaxed);
    state_.store(TransportState::Stopped,std::memory_order_release);
}

void AudioSession::apply_commands() noexcept {
    AudioCommand command;
    while(commands_.pop(command)) {
        switch(command.type) {
        case AudioCommandType::Play:
            if(position_>=total_frames()) position_=0;
            state_.store(TransportState::Playing,std::memory_order_release);
            break;
        case AudioCommandType::Pause:
            if(state_.load(std::memory_order_relaxed)==TransportState::Playing)
                state_.store(TransportState::Paused,std::memory_order_release);
            break;
        case AudioCommandType::Stop:
            position_=0;
            engine_.reset();
            state_.store(TransportState::Stopped,std::memory_order_release);
            break;
        case AudioCommandType::SetLoop: loop_=command.value>=0.5f; break;
        case AudioCommandType::SetMorph: params_.morph=std::clamp(command.value,0.0f,1.0f); break;
        case AudioCommandType::SetMorphTime: params_.morph_time_ms=std::clamp(command.value,1.0f,5000.0f); break;
        case AudioCommandType::SetWet: params_.wet=std::clamp(command.value,0.0f,1.0f); break;
        case AudioCommandType::SetInputGain: input_gain_=std::clamp(command.value,0.0f,2.0f); break;
        case AudioCommandType::SetOutputGain: output_gain_=std::clamp(command.value,0.0f,2.0f); break;
        }
    }
}

void AudioSession::render(float* output,unsigned frames) noexcept {
    if(!output || !frames || frames>max_block_frames) return;
    std::fill_n(output,std::size_t(frames)*2,0.0f);
    apply_commands();
    if(state_.load(std::memory_order_relaxed)!=TransportState::Playing || total_frames()==0) {
        peak_left_.store(0,std::memory_order_relaxed);
        peak_right_.store(0,std::memory_order_relaxed);
        position_snapshot_.store(position_,std::memory_order_relaxed);
        return;
    }
    unsigned active=0;
    const auto total=total_frames();
    while(active<frames) {
        if(position_>=total) {
            if(loop_) position_=0;
            else {
                state_.store(TransportState::Stopped,std::memory_order_release);
                break;
            }
        }
        const auto source=position_*2;
        output[2*active]=source_.samples[source]*input_gain_;
        output[2*active+1]=source_.samples[source+1]*input_gain_;
        ++active;
        ++position_;
    }
    if(position_>=total && !loop_) state_.store(TransportState::Stopped,std::memory_order_release);
    if(active) engine_.process(output,active,params_);
    morph_snapshot_.store(engine_.current_morph(),std::memory_order_relaxed);
    float peak_l=0,peak_r=0;
    for(unsigned i=0;i<frames;++i) {
        const auto l=std::clamp(std::isfinite(output[2*i])?output[2*i]*output_gain_:0.0f,-1.0f,1.0f);
        const auto r=std::clamp(std::isfinite(output[2*i+1])?output[2*i+1]*output_gain_:0.0f,-1.0f,1.0f);
        output[2*i]=l; output[2*i+1]=r;
        peak_l=std::max(peak_l,std::abs(l)); peak_r=std::max(peak_r,std::abs(r));
    }
    peak_left_.store(peak_l,std::memory_order_relaxed);
    peak_right_.store(peak_r,std::memory_order_relaxed);
    position_snapshot_.store(position_,std::memory_order_relaxed);
}

AudioBuffer render_offline(const AudioBuffer& source,const MorphParams& params,float input_gain,float output_gain) {
    if(source.channels!=2 || !source.sample_rate || source.samples.size()%2)
        throw std::invalid_argument("offline render requires valid stereo audio");
    AudioBuffer result=source;
    MorphEngine engine; engine.prepare(source.sample_rate,AudioSession::max_block_frames);
    input_gain=std::clamp(input_gain,0.0f,2.0f); output_gain=std::clamp(output_gain,0.0f,2.0f);
    const auto frames=static_cast<unsigned>(result.samples.size()/2);
    for(unsigned start=0;start<frames;) {
        const auto count=std::min(AudioSession::max_block_frames,frames-start);
        auto* block=result.samples.data()+std::size_t(start)*2;
        for(unsigned i=0;i<count*2;++i) block[i]*=input_gain;
        engine.process(block,count,params);
        for(unsigned i=0;i<count*2;++i)
            block[i]=std::clamp(std::isfinite(block[i])?block[i]*output_gain:0.0f,-1.0f,1.0f);
        start+=count;
    }
    return result;
}
}
