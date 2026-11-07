#include "Maze.Soundscape.h"
#include <algorithm>
#include <cmath>

namespace { constexpr float pi = 3.14159265358979323846f; }

SoundscapeSynth::SoundscapeSynth(unsigned int sampleRate)
    : sampleRate_(sampleRate ? sampleRate : 22050)
{
}

void SoundscapeSynth::render(std::int16_t* output, std::size_t count, const SoundscapeFrame& frame)
{
    if (!output) return;
    if (frame.thunderEvent != thunderEvent_)
    {
        if (frame.thunderEvent != 0 && frame.active) thunderDelay_ = 1.2f;
        thunderEvent_ = frame.thunderEvent;
    }
    if (frame.active && frame.hunterChasing && !wasChasing_ && frame.threat > .05f) roar_ = 1.f;
    wasChasing_ = frame.active && frame.hunterChasing;
    const float dt = 1.f / static_cast<float>(sampleRate_);
    const float threat = std::clamp(frame.threat, 0.f, 1.f);
    const float shifting = std::clamp(frame.shifting, 0.f, 1.f);
    const float hunterPan = std::clamp(frame.hunterPan, -1.f, 1.f);
    const float shiftingPan = std::clamp(frame.shiftingPan, -1.f, 1.f);
    const float hunterPeriod = std::clamp(frame.hunterStepSeconds, .3f, 2.f);
    for (std::size_t i = 0; i < count; ++i)
    {
        random_ ^= random_ << 13; random_ ^= random_ >> 17; random_ ^= random_ << 5;
        const float noise = static_cast<float>(random_ & 0xffffu) / 32767.5f - 1.f;
        wind_ += .0015f * (noise - wind_);
        rumble_ += .010f * (noise - rumble_);
        breathNoise_ += .09f * (noise - breathNoise_);
        const float gust = .65f + .35f * std::sin(time_ * .19f);
        const float drone = std::sin(time_ * 2.f * pi * 46.f) * .012f;
        float sample = wind_ * 1.1f * gust + drone;
        if (frame.moving)
        {
            const float period = frame.running ? .34f : .53f;
            footPhase_ += dt;
            if (footPhase_ >= period) footPhase_ -= period;
            const float envelope = std::exp(-footPhase_ * 32.f);
            sample += envelope * (.10f * std::sin(footPhase_ * 2.f * pi * 62.f) + .03f * noise);
        }
        else footPhase_ = 0.f;
        heartPhase_ += dt;
        const float heartPeriod = 1.25f - .35f * threat;
        if (heartPhase_ >= heartPeriod) heartPhase_ -= heartPeriod;
        const float first = std::exp(-heartPhase_ * 24.f);
        const float second = heartPhase_ >= .20f ? std::exp(-(heartPhase_ - .20f) * 30.f) * .7f : 0.f;
        sample += threat * .11f * (first + second) * std::sin(heartPhase_ * 2.f * pi * 47.f);
        hunterPan_ += .0015f * (hunterPan - hunterPan_);
        shiftingPan_ += .0015f * (shiftingPan - shiftingPan_);
        if (frame.hunterMoving)
        {
            hunterPhase_ += dt;
            if (hunterPhase_ >= hunterPeriod) hunterPhase_ -= hunterPeriod;
        }
        else hunterPhase_ = 0.f;
        const float hunterStep = frame.hunterMoving ? std::exp(-hunterPhase_ * 12.f) : 0.f;
        const float breathRate = frame.hunterChasing ? 1.3f : .58f;
        breathPhase_ += dt * breathRate;
        if (breathPhase_ >= 1.f) breathPhase_ -= 1.f;
        const float breathEnvelope = std::pow(std::max(0.f, std::sin(breathPhase_ * 2.f * pi)), 1.5f);
        const float roughness = .70f + .30f * std::sin(time_ * 29.f);
        const float vocalFrequency = (frame.hunterChasing ? 91.f : 72.f) + 7.f * std::sin(time_ * 2.3f);
        vocalPhase_ += dt * vocalFrequency;
        if (vocalPhase_ >= 1.f) vocalPhase_ -= 1.f;
        const float growl = std::sin(vocalPhase_ * 2.f * pi) * .52f
            + std::sin(vocalPhase_ * 4.f * pi) * .25f + std::sin(vocalPhase_ * 6.f * pi) * .11f;
        const float voice = breathEnvelope * (.14f * breathNoise_ + .075f * growl * roughness)
            + roar_ * (.18f * growl + .13f * breathNoise_);
        roar_ *= std::exp(-dt / .75f);
        const float creature = threat * (hunterStep * (.12f * std::sin(hunterPhase_ * 2.f * pi * 51.f)
            + .12f * rumble_) + .045f * rumble_ + voice);
        const float branches = shifting * .18f * rumble_;
        if (thunderDelay_ > 0.f)
        {
            thunderDelay_ -= dt;
            if (thunderDelay_ <= 0.f) thunder_ = 1.f;
        }
        sample += thunder_ * .48f * wind_;
        thunder_ *= std::exp(-dt / 1.8f);
        gain_ += .002f * ((frame.active ? 1.f : 0.f) - gain_);
        const float left = sample + creature * (1.f - hunterPan_) + branches * (1.f - shiftingPan_);
        const float right = sample + creature * (1.f + hunterPan_) + branches * (1.f + shiftingPan_);
        output[2 * i] = static_cast<std::int16_t>(std::clamp(left * gain_, -1.f, 1.f) * 32767.f);
        output[2 * i + 1] = static_cast<std::int16_t>(std::clamp(right * gain_, -1.f, 1.f) * 32767.f);
        time_ += dt;
        if (time_ >= 1000.f) time_ -= 1000.f;
    }
}
