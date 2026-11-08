#include "pch.h"
#include "CppUnitTest.h"
#include "Maze.Atmosphere.h"
#include <cmath>
#include <limits>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace MazeUnitTest
{
    TEST_CLASS(MazeAtmosphereTests)
    {
    public:
        TEST_METHOD(DarknessIsSeededBoundedAndContinuous)
        {
            bool variesByPosition = false, variesBySeed = false;
            const float origin = hedgeDarkness(0.f, 0.f, 42);
            for (int row = 0; row < 50; ++row) for (int col = 0; col < 50; ++col)
            {
                const float a = hedgeDarkness(static_cast<float>(col), static_cast<float>(row), 42);
                const float b = hedgeDarkness(col + .01f, row + .01f, 42);
                Assert::IsTrue(a >= 0.f && a <= 1.f && std::abs(a - b) < .03f);
                Assert::AreEqual(a, hedgeDarkness(static_cast<float>(col), static_cast<float>(row), 42));
                variesByPosition = variesByPosition || std::abs(a - origin) > .05f;
                variesBySeed = variesBySeed || std::abs(a - hedgeDarkness(static_cast<float>(col), static_cast<float>(row), 43)) > .05f;
            }
            Assert::IsTrue(variesByPosition && variesBySeed);
            Assert::AreEqual(0.f, hedgeDarkness(std::numeric_limits<float>::quiet_NaN(), 0.f, 1));
        }

        TEST_METHOD(StormStrikesAreSeededAndEventsFollowTheFlashes)
        {
            bool differentSeeds = false;
            for (std::uint32_t seed = 0; seed < 16; ++seed)
            {
                std::uint32_t previous = 0;
                float lastStrike = -100.f;
                int brightSamples = 0;
                for (int tick = 0; tick < 17600; ++tick)
                {
                    const float time = tick * .01f;
                    const auto storm = sampleStorm(time, seed);
                    const auto repeat = sampleStorm(time, seed);
                    Assert::AreEqual(storm.event, repeat.event);
                    Assert::AreEqual(storm.flash, repeat.flash);
                    Assert::IsTrue(storm.flash >= 0.f && storm.flash <= 1.f);
                    Assert::IsTrue(storm.event >= previous && storm.event <= previous + 1);
                    if (storm.event != previous)
                    {
                        if (previous != 0) Assert::IsTrue(time - lastStrike >= 20.f && time - lastStrike <= 68.f);
                        previous = storm.event; lastStrike = time;
                    }
                    if (storm.flash > .1f)
                    {
                        ++brightSamples;
                        Assert::IsTrue(storm.event > 0 && time - lastStrike < .8f);
                    }
                    differentSeeds = differentSeeds || storm.event != sampleStorm(time, seed + 1).event;
                }
                Assert::AreEqual(static_cast<std::uint32_t>(4), previous);
                Assert::IsTrue(brightSamples > 20);
            }
            Assert::IsTrue(differentSeeds);
            Assert::AreEqual(0u, sampleStorm(-1.f, 42).event);
            Assert::AreEqual(0.f, sampleStorm(std::numeric_limits<float>::quiet_NaN(), 42).flash);
        }
    };
}
