#include "pch.h"
#include "CppUnitTest.h"
#include "Maze.CreatureMesh.h"
#include <limits>
#include <sstream>
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace MazeUnitTest
{
    namespace
    {
        std::string meshBytes(bool badIndex = false, bool nonfinite = false)
        {
            std::ostringstream output(std::ios::binary);
            const std::uint32_t count = 3;
            CreatureVertex vertices[3]{};
            vertices[0].position[0] = nonfinite ? std::numeric_limits<float>::quiet_NaN() : 0.f;
            vertices[1].position[0] = 1.f; vertices[2].position[1] = 1.f;
            const std::uint32_t indices[] = {0, 1, badIndex ? 3u : 2u};
            output.write("NXM1", 4);
            output.write(reinterpret_cast<const char*>(&count), sizeof(count));
            output.write(reinterpret_cast<const char*>(&count), sizeof(count));
            output.write(reinterpret_cast<const char*>(vertices), sizeof(vertices));
            output.write(reinterpret_cast<const char*>(indices), sizeof(indices));
            return output.str();
        }
        bool parses(const std::string& bytes)
        {
            std::istringstream input(bytes, std::ios::binary);
            return readCreatureMesh(input).has_value();
        }
        std::string animationBytes(bool badIndex = false, bool nonfinite = false)
        {
            std::ostringstream output(std::ios::binary);
            const std::uint32_t vertices = 3, indicesCount = 3, frameCount = 2;
            const float duration = 1.f;
            const float uv[6] = {0,0, 1,0, 0,1};
            const std::uint32_t indices[] = {0, 1, badIndex ? 3u : 2u};
            AnimatedVertex frame[3]{}; frame[1].position[0] = nonfinite ? std::numeric_limits<float>::quiet_NaN() : 1.f;
            output.write("NXA1", 4);
            for (auto value : {vertices, indicesCount, frameCount}) output.write(reinterpret_cast<const char*>(&value), sizeof(value));
            output.write(reinterpret_cast<const char*>(&duration), sizeof(duration));
            output.write(reinterpret_cast<const char*>(uv), sizeof(uv));
            output.write(reinterpret_cast<const char*>(indices), sizeof(indices));
            output.write(reinterpret_cast<const char*>(frame), sizeof(frame));
            frame[2].position[1] = 1.f;
            output.write(reinterpret_cast<const char*>(frame), sizeof(frame));
            return output.str();
        }
    }
    TEST_CLASS(CreatureMeshTests)
    {
    public:
        TEST_METHOD(ReadsAndValidatesAnimatedMeshFrames)
        {
            const auto valid = animationBytes();
            std::istringstream input(valid, std::ios::binary);
            const auto mesh = readAnimatedMesh(input);
            Assert::IsTrue(mesh.has_value());
            Assert::AreEqual(static_cast<std::size_t>(2), mesh->frames.size());
            Assert::AreEqual(1.f, mesh->frames[1][2].position[1]);
            for (const auto& bad : { animationBytes(true), animationBytes(false, true), valid.substr(0, valid.size()-1), valid + "x" })
            {
                std::istringstream malformed(bad, std::ios::binary);
                Assert::IsFalse(readAnimatedMesh(malformed).has_value());
            }
        }
        TEST_METHOD(ReadsValidTriangle)
        {
            std::istringstream input(meshBytes(), std::ios::binary);
            const auto mesh = readCreatureMesh(input);
            Assert::IsTrue(mesh.has_value());
            Assert::AreEqual(static_cast<std::size_t>(3), mesh->vertices.size());
            Assert::AreEqual(static_cast<std::uint32_t>(2), mesh->indices[2]);
        }
        TEST_METHOD(RejectsCorruptOrOversizedData)
        {
            Assert::IsFalse(parses(meshBytes(true)));
            Assert::IsFalse(parses(meshBytes(false, true)));
            const auto valid = meshBytes();
            Assert::IsFalse(parses(valid.substr(0, valid.size() - 1)));
            Assert::IsFalse(parses(valid + "x"));
            auto badMagic = valid; badMagic[0] = 'X'; Assert::IsFalse(parses(badMagic));
            std::ostringstream oversized(std::ios::binary);
            const std::uint32_t huge = 200001, count = 3;
            oversized.write("NXM1", 4);
            oversized.write(reinterpret_cast<const char*>(&huge), sizeof(huge));
            oversized.write(reinterpret_cast<const char*>(&count), sizeof(count));
            Assert::IsFalse(parses(oversized.str()));
        }
    };
}
