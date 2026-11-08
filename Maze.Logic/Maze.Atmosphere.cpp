#include "Maze.Atmosphere.h"
#include <algorithm>
#include <cmath>

namespace
{
    float value(int x, int z, std::uint32_t seed) noexcept
    {
        std::uint32_t hash = seed ^ (static_cast<std::uint32_t>(x) * 0x8da6b343u)
            ^ (static_cast<std::uint32_t>(z) * 0xd8163841u);
        hash ^= hash >> 16; hash *= 0x7feb352du;
        hash ^= hash >> 15; hash *= 0x846ca68bu; hash ^= hash >> 16;
        return static_cast<float>(hash & 0xffffu) / 65535.f;
    }
    float smooth(float t) noexcept { return t * t * (3.f - 2.f * t); }
    std::uint32_t stormHash(std::uint32_t value) noexcept
    {
        value ^= value >> 16; value *= 0x7feb352du;
        value ^= value >> 15; value *= 0x846ca68bu; value ^= value >> 16;
        return value;
    }
    float pulse(float age, float start, float duration) noexcept
    {
        const float phase = (age - start) / duration;
        return phase > 0.f && phase < 1.f ? std::sin(phase * 3.14159265358979323846f) : 0.f;
    }
}

StormSample sampleStorm(float seconds, std::uint32_t seed) noexcept
{
    if (!std::isfinite(seconds) || seconds < 0.f) return {};
    // Limiting the time also keeps the float-to-integer conversion well-defined.
    constexpr float period = 44.f;
    seconds = std::min(seconds, 100000000.f);
    const auto slot = static_cast<std::uint32_t>(seconds / period);
    const float within = seconds - static_cast<float>(slot) * period;
    const auto hash = stormHash(seed ^ (slot * 0x9e3779b9u) ^ 0x83d2e9b5u);
    const float strike = 10.f + static_cast<float>(hash & 0xffffu) / 65535.f * 23.f;
    const float age = within - strike;
    StormSample result;
    result.event = slot + (age >= 0.f ? 1u : 0u);
    if (age >= 0.f && age < .8f)
    {
        const float delay = .20f + static_cast<float>((hash >> 16) & 255u) / 255.f * .12f;
        result.flash = std::max(pulse(age, 0.f, .13f), .65f * pulse(age, delay, .15f));
        if (hash & 0x1000000u)
            result.flash = std::max(result.flash, .33f * pulse(age, delay + .24f, .12f));
    }
    return result;
}

float hedgeDarkness(float x, float z, std::uint32_t seed) noexcept
{
    if (!std::isfinite(x) || !std::isfinite(z)) return 0.f;
    // World coordinates are bounded by the maze; clamp protects callers that
    // provide arbitrary finite input before conversion to integer grid cells.
    x = std::clamp(x, -10000.f, 10000.f) / 6.f;
    z = std::clamp(z, -10000.f, 10000.f) / 6.f;
    const int ix = static_cast<int>(std::floor(x)), iz = static_cast<int>(std::floor(z));
    const float u = smooth(x - ix), v = smooth(z - iz);
    const float a = value(ix, iz, seed) * (1.f - u) + value(ix + 1, iz, seed) * u;
    const float b = value(ix, iz + 1, seed) * (1.f - u) + value(ix + 1, iz + 1, seed) * u;
    return std::clamp((a * (1.f - v) + b * v - .40f) * 2.5f, 0.f, 1.f);
}
