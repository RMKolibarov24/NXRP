#include "pch.h"
#include "CppUnitTest.h"
#include "Maze.Soundscape.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MazeUnitTest
{
    TEST_CLASS(SoundscapeTests)
    {
    public:
        TEST_METHOD(ChasingCreatureVocalizesEvenWhenNotStepping)
        {
            SoundscapeSynth calm, chasing;
            SoundscapeFrame frame; frame.active = true; frame.threat = .8f; frame.hunterPan = 1.f;
            SoundscapeFrame chase = frame; chase.hunterChasing = true;
            std::array<std::int16_t, 8192> quiet{}, alarm{};
            calm.render(quiet.data(), quiet.size()/2, frame);
            chasing.render(alarm.data(), alarm.size()/2, chase);
            long long difference = 0, stereoDifference = 0;
            for (std::size_t i = 0; i < quiet.size(); i += 2)
            {
                difference += std::abs(static_cast<int>(alarm[i+1]) - static_cast<int>(quiet[i+1]));
                stereoDifference += std::abs(static_cast<int>(alarm[i+1]) - static_cast<int>(alarm[i]));
            }
            Assert::IsTrue(difference > 50000);
            Assert::IsTrue(stereoDifference > 50000);
        }
        TEST_METHOD(SilenceAndPauseFadeDoNotLeaveResidualOutput)
        {
            SoundscapeSynth synth;
            SoundscapeFrame frame;
            std::vector<std::int16_t> samples(88200);
            synth.render(samples.data(), samples.size() / 2, frame);
            Assert::IsTrue(std::all_of(samples.begin(), samples.end(), [](auto s) { return s == 0; }));
            frame.active = true; frame.moving = true;
            synth.render(samples.data(), samples.size() / 2, frame);
            Assert::IsTrue(std::any_of(samples.begin(), samples.end(), [](auto s) { return s != 0; }));
            frame.active = false;
            synth.render(samples.data(), samples.size() / 2, frame);
            Assert::IsTrue(std::all_of(samples.end() - 512, samples.end(), [](auto s) { return s == 0; }));
        }

        TEST_METHOD(ContinuousBlocksMatchOneRender)
        {
            SoundscapeFrame frame; frame.active = true; frame.moving = true;
            frame.running = true; frame.threat = .7f; frame.shifting = .4f; frame.thunderEvent = 1;
            SoundscapeSynth one, split;
            std::array<std::int16_t, 4096> full{}, parts{};
            one.render(full.data(), full.size() / 2, frame);
            split.render(parts.data(), 512, frame);
            split.render(parts.data() + 1024, parts.size() / 2 - 512, frame);
            Assert::IsTrue(full == parts);
        }

        TEST_METHOD(ThreatAndWeatherChangeTheOutput)
        {
            SoundscapeSynth quiet, danger;
            SoundscapeFrame base; base.active = true;
            SoundscapeFrame alarm = base; alarm.threat = 1.f; alarm.shifting = 1.f; alarm.thunderEvent = 1;
            std::array<std::int16_t, 4096> first{}, second{};
            quiet.render(first.data(), first.size() / 2, base);
            danger.render(second.data(), second.size() / 2, alarm);
            Assert::IsFalse(first == second);
        }

        TEST_METHOD(DirectionalSoundsPanWithoutChangingTheCenteredAmbience)
        {
            SoundscapeSynth leftSynth, rightSynth, centerSynth;
            SoundscapeFrame left; left.active = true; left.threat = .8f;
            left.hunterMoving = true; left.hunterPan = -1.f;
            left.shifting = .6f; left.shiftingPan = -1.f;
            SoundscapeFrame right = left; right.hunterPan = right.shiftingPan = 1.f;
            SoundscapeFrame center = left; center.hunterPan = center.shiftingPan = 0.f;
            std::array<std::int16_t, 8192> l{}, r{}, c{};
            leftSynth.render(l.data(), l.size() / 2, left);
            rightSynth.render(r.data(), r.size() / 2, right);
            centerSynth.render(c.data(), c.size() / 2, center);
            bool separated = false;
            for (std::size_t i = 0; i < l.size(); i += 2)
            {
                Assert::AreEqual(l[i], r[i + 1]);
                Assert::AreEqual(l[i + 1], r[i]);
                Assert::AreEqual(c[i], c[i + 1]);
                separated = separated || l[i] != l[i + 1];
            }
            Assert::IsTrue(separated);
        }
    };
}
