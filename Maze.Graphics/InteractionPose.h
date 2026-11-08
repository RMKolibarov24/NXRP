#pragma once
#include "Maze.Safehouse.h"
#include <algorithm>
#include <cmath>

struct InteractionPoint { float x, y, z; };
inline float interactionSmooth(float value) noexcept
{
    value=std::clamp(value,0.f,1.f);
    return value*value*value*(value*(value*6.f-15.f)+10.f);
}
inline float slotLeverDegrees(float time, float stopTime) noexcept
{
    const float t=time-(stopTime-2.f);
    if(t<.30f || t>1.65f) return 0.f;
    if(t<.85f)return -65.f*interactionSmooth((t-.30f)/.55f);
    if(t<.98f)return -65.f;
    const float u=std::clamp((t-.98f)/.67f,0.f,1.f);
    const float spring=3.5f*std::sin(u*12.5663706f)*std::pow(std::sin(u*3.14159265f),2.f);
    return -65.f*(1.f-interactionSmooth(u))+spring;
}
inline float slotHandReach(float time,float stopTime) noexcept
{
    const float t=time-(stopTime-2.f);
    if(t<0.f || t>1.95f)return 0.f;
    if(t<.30f)return interactionSmooth(t/.30f);
    return t<1.65f?1.f:1.f-interactionSmooth((t-1.65f)/.30f);
}
inline float slotReelTravel(float time,float stopTime,int reel,int result) noexcept
{
    // The reels respond to the physical pull, after the hand has made contact.
    const float duration=2.f-(2-reel)*.25f-.75f;
    const float u=std::clamp((time-(stopTime-2.f)-.75f)/duration,0.f,1.f);
    return (12.f+result)*(1.f-std::pow(1.f-u,3.f));
}
inline float slotHandGrip(float time,float stopTime) noexcept
{
    const float t=time-(stopTime-2.f);
    return interactionSmooth((t-.18f)/.12f)*(1.f-interactionSmooth((t-1.62f)/.16f));
}
inline InteractionPoint slotKnobPoint(MazePosition house,float time,float stopTime) noexcept
{
    const float angle=slotLeverDegrees(time,stopTime)*.01745329252f;
    const float y=.1943f*std::cos(angle)+.0134f*std::sin(angle);
    const float z=.1943f*std::sin(angle)-.0134f*std::cos(angle);
    return {house.col+.5f+(-.85f+(.6701586f+.0415f)*.45f)*refugeScale,
        .6501679f+y,house.row+.5f+(-.85f+(-.1783184f+z)*.45f)*refugeScale};
}
