#include "morph_engine.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace vortex {
void MorphEngine::prepare(unsigned sr,unsigned) { if(sr<8000) throw std::invalid_argument("sample rate too low"); rate_=sr; const auto n=sr*2u+4u; left_.assign(n,0); right_.assign(n,0); cursor_=0; morph_current_=0.5f; }
void MorphEngine::process(float* x,unsigned frames,const MorphParams& p) {
 if(left_.empty()) prepare(rate_,frames);
 const float target=std::clamp(p.morph,0.0f,1.0f), wet=std::clamp(p.wet,0.0f,1.0f);
 const float smoothing=1.0f-std::exp(-1.0f/(0.025f*rate_));
 const float max_delay=float(left_.size()-3);
 for(unsigned i=0;i<frames;++i) {
   morph_current_+=smoothing*(target-morph_current_);
   const float m=morph_current_;
   const float fb=std::clamp(p.feedback_a+(p.feedback_b-p.feedback_a)*m,0.0f,0.92f);
   const float delay=std::clamp((p.delay_a_ms+(p.delay_b_ms-p.delay_a_ms)*m)*0.001f*rate_,1.0f,max_delay);
   const auto whole=std::size_t(delay); const float fraction=delay-float(whole);
   const std::size_t r0=(std::size_t(cursor_)+left_.size()-whole)%left_.size();
   const std::size_t r1=(r0+left_.size()-1)%left_.size();
   float inL=std::isfinite(x[2*i])?x[2*i]:0, inR=std::isfinite(x[2*i+1])?x[2*i+1]:0;
   float dl=left_[r0]*(1.0f-fraction)+left_[r1]*fraction;
   float dr=right_[r0]*(1.0f-fraction)+right_[r1]*fraction;
   left_[cursor_]=std::clamp(inL+dr*fb,-4.0f,4.0f); right_[cursor_]=std::clamp(inR+dl*fb,-4.0f,4.0f);
   x[2*i]=inL*(1-wet)+dl*wet; x[2*i+1]=inR*(1-wet)+dr*wet;
   cursor_=(cursor_+1)%unsigned(left_.size());
 }
}
}
