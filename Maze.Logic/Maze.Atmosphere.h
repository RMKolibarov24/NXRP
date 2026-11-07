#pragma once
#include <cstdint>

// Smooth seeded field in [0,1] for finite world coordinates. Nearby points vary
// gradually, so entering a dark part of the hedge does not cause a hard cut.
float hedgeDarkness(float x, float z, std::uint32_t seed) noexcept;

struct StormSample
{
    float flash = 0.f;
    std::uint32_t event = 0;
};

// A reproducible sequence of unevenly spaced lightning strikes. event changes
// once at the start of each strike and stays stable between flashes.
StormSample sampleStorm(float seconds, std::uint32_t seed) noexcept;
