#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <string>

// 20ms of 16kHz mono speech. Independent mu-law samples keep packet loss and
// queue drops from corrupting subsequent speech, with no decoder history.
constexpr std::size_t voiceSamples=320;
constexpr unsigned voiceRate=16000;
struct VoicePacket
{
    unsigned generation=0;
    int actor=0;
    std::uint32_t sequence=0;
    std::array<std::uint8_t,voiceSamples> samples{};
};

inline std::uint8_t voiceEncodeSample(std::int16_t sample) noexcept
{
    int value=sample;
    const int sign=value<0?0x80:0;
    if(value<0)value=-value;
    value=std::min(value,32635)+132;
    int exponent=7;
    for(int mask=0x4000;exponent>0 && !(value&mask);mask>>=1)--exponent;
    const int mantissa=(value>>(exponent+3))&15;
    return static_cast<std::uint8_t>(~(sign|(exponent<<4)|mantissa));
}
inline std::int16_t voiceDecodeSample(std::uint8_t sample) noexcept
{
    const unsigned value=static_cast<unsigned>(~sample)&255u;
    int magnitude=static_cast<int>(((value&15u)<<3)+132u)<<((value>>4)&7u);
    magnitude-=132;
    return static_cast<std::int16_t>((value&128u)?-magnitude:magnitude);
}
inline std::string serializeVoice(const VoicePacket& packet)
{
    static constexpr char hex[]="0123456789abcdef";
    std::string payload;payload.reserve(voiceSamples*2);
    for(const auto sample:packet.samples){payload+=hex[sample>>4];payload+=hex[sample&15];}
    return "V "+std::to_string(packet.generation)+" "+std::to_string(packet.actor)+" "+std::to_string(packet.sequence)+" "+payload;
}
inline bool parseVoice(const std::string& message,VoicePacket& packet)
{
    // Exact-size payload and numeric bounds are checked before publishing data.
    if(message.size()>720 || message.find('-')!=std::string::npos)return false;
    std::istringstream input(message);char type;std::string payload,extra;
    VoicePacket candidate;
    if(!(input>>type>>candidate.generation>>candidate.actor>>candidate.sequence>>payload) || type!='V' ||
        candidate.actor<0 || candidate.actor>3 || payload.size()!=voiceSamples*2 || (input>>extra))return false;
    const auto digit=[](char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;};
    for(std::size_t i=0;i<voiceSamples;++i)
    {
        const int high=digit(payload[i*2]),low=digit(payload[i*2+1]);
        if(high<0 || low<0)return false;
        candidate.samples[i]=static_cast<std::uint8_t>(high*16+low);
    }
    packet=candidate;return true;
}
inline float voiceLevel(const VoicePacket& packet) noexcept
{
    double energy=0.;
    for(const auto value:packet.samples){const double sample=voiceDecodeSample(value)/32768.;energy+=sample*sample;}
    return static_cast<float>(std::sqrt(energy/voiceSamples));
}
