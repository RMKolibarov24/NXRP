#pragma once
#include "Maze.Safehouse.h"
#include <algorithm>
#include <cmath>

struct InteractionPoint { float x, y, z; };
inline float interactionSmooth(float value) noexcept
{
    value=std::clamp(value,0.f,1.f);
    return value*value*(3.f-2.f*value);
}
inline float slotLeverDegrees(float time, float stopTime) noexcept
{
    const float t=time-(stopTime-2.f);
    if(t<.25f || t>1.f) return 0.f;
    return -65.f*(t<.65f?interactionSmooth((t-.25f)/.4f):1.f-interactionSmooth((t-.65f)/.35f));
}
inline InteractionPoint slotKnobPoint(MazePosition house,float time,float stopTime) noexcept
{
    const float angle=slotLeverDegrees(time,stopTime)*.01745329252f;
    const float y=.1943f*std::cos(angle)+.0134f*std::sin(angle);
    const float z=.1943f*std::sin(angle)-.0134f*std::cos(angle);
    return {house.col+.5f+(-.85f+(.6701586f+.0415f)*.45f)*refugeScale,
        .6501679f+y,house.row+.5f+(-.85f+(-.1783184f+z)*.45f)*refugeScale};
}
