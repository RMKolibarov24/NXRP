#pragma once

#include <cstddef>
#include <cstdint>

struct SoundscapeFrame
{
    bool active = false;
    bool moving = false;
    bool running = false;
    float threat = 0.f;
    float shifting = 0.f;
    float hunterPan = 0.f;
    float shiftingPan = 0.f;
    bool hunterMoving = false;
    bool hunterChasing = false;
    float hunterStepSeconds = 1.f;
    std::uint32_t thunderEvent = 0;
};

// Stateful, allocation-free interleaved stereo PCM synthesis. count is the number
// of frames; output must have room for 2 * count samples. The caller may update parameters
// between blocks; audio playback and device management live in the graphics layer.
class SoundscapeSynth
{
public:
    explicit SoundscapeSynth(unsigned int sampleRate = 22050);
    void render(std::int16_t* output, std::size_t count, const SoundscapeFrame& frame);
private:
    unsigned int sampleRate_;
    std::uint32_t random_ = 0x9e3779b9u, thunderEvent_ = 0;
    float time_ = 0.f, footPhase_ = 0.f, heartPhase_ = 0.f, hunterPhase_ = 0.f;
    float wind_ = 0.f, rumble_ = 0.f, thunder_ = 0.f, thunderDelay_ = 0.f, gain_ = 0.f;
    float hunterPan_ = 0.f, shiftingPan_ = 0.f;
    float breathNoise_ = 0.f, vocalPhase_ = 0.f, breathPhase_ = 0.f, roar_ = 0.f;
    bool wasChasing_ = false;
};
