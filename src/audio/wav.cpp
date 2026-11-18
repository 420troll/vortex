#include "wav.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <stdexcept>

namespace vortex {
namespace {
std::uint16_t u16(const unsigned char* p) { return std::uint16_t(p[0] | (p[1] << 8)); }
std::uint32_t u32(const unsigned char* p) { return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) | (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24); }
void put16(std::ostream& o, std::uint16_t x) { char b[2]{char(x), char(x >> 8)}; o.write(b, 2); }
void put32(std::ostream& o, std::uint32_t x) { char b[4]{char(x), char(x >> 8), char(x >> 16), char(x >> 24)}; o.write(b, 4); }
}
AudioBuffer read_wav(const std::string& path) {
    std::ifstream f(path, std::ios::binary); if (!f) throw std::runtime_error("cannot open input WAV");
    unsigned char h[12]; f.read(reinterpret_cast<char*>(h), 12);
    if (f.gcount()!=12 || std::memcmp(h,"RIFF",4) || std::memcmp(h+8,"WAVE",4)) throw std::runtime_error("not a RIFF/WAVE file");
    std::uint16_t format=0, channels=0, bits=0; std::uint32_t rate=0; std::vector<unsigned char> data;
    while (f) {
        unsigned char ch[8]; f.read(reinterpret_cast<char*>(ch),8); if (f.gcount()!=8) break;
        auto n=u32(ch+4); if (n > 0x40000000u) throw std::runtime_error("WAV chunk too large");
        if (!std::memcmp(ch,"fmt ",4)) { std::vector<unsigned char> b(n); f.read(reinterpret_cast<char*>(b.data()),n); if(n<16) throw std::runtime_error("short fmt chunk"); format=u16(b.data()); channels=u16(b.data()+2); rate=u32(b.data()+4); bits=u16(b.data()+14); }
        else if (!std::memcmp(ch,"data",4)) { data.resize(n); f.read(reinterpret_cast<char*>(data.data()),n); }
        else f.seekg(n,std::ios::cur);
        if(n&1) f.seekg(1,std::ios::cur);
    }
    if ((format!=1 && format!=3) || (channels!=1 && channels!=2) || !rate || data.empty()) throw std::runtime_error("unsupported or incomplete WAV (PCM/float mono or stereo required)");
    if (!((format==1 && (bits==16 || bits==24 || bits==32)) || (format==3 && bits==32))) throw std::runtime_error("unsupported WAV encoding");
    const unsigned width=bits/8; if(data.size()% (width*channels)) throw std::runtime_error("partial WAV frame");
    AudioBuffer a{rate,channels,{}}; a.samples.reserve(data.size()/width);
    for(size_t i=0;i<data.size();i+=width) {
        float x=0;
        if(format==3) std::memcpy(&x,data.data()+i,4);
        else if(bits==16) x=float(std::int16_t(u16(data.data()+i)))/32768.0f;
        else if(bits==24) { std::int32_t v=data[i]|(data[i+1]<<8)|(data[i+2]<<16); if(v&0x800000)v|=~0xffffff; x=float(v)/8388608.0f; }
        else x=float(std::int32_t(u32(data.data()+i)))/2147483648.0f;
        a.samples.push_back(std::isfinite(x)?x:0.0f);
    }
    return a;
}
void write_wav(const std::string& path,const AudioBuffer& a) {
    if ((a.channels!=1 && a.channels!=2) || !a.sample_rate || a.samples.size()%a.channels) throw std::runtime_error("invalid audio buffer");
    if(a.samples.size() > (0xffffffffu-36)/2) throw std::runtime_error("WAV too large");
    std::ofstream f(path,std::ios::binary); if(!f) throw std::runtime_error("cannot open output WAV");
    const auto bytes=std::uint32_t(a.samples.size()*2);
    f.write("RIFF",4); put32(f,36+bytes); f.write("WAVEfmt ",8); put32(f,16); put16(f,1); put16(f,a.channels); put32(f,a.sample_rate); put32(f,a.sample_rate*a.channels*2); put16(f,a.channels*2); put16(f,16); f.write("data",4); put32(f,bytes);
    for(float s:a.samples) { float c=std::clamp(std::isfinite(s)?s:0.0f,-1.0f,0.999969f); put16(f,std::uint16_t(std::int16_t(std::lrint(c*32768.0f)))); }
    if(!f) throw std::runtime_error("failed writing WAV");
}
}
