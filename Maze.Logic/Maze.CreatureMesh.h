#pragma once
#include <cstdint>
#include <array>
#include <iosfwd>
#include <optional>
#include <vector>

struct CreatureVertex
{
    float position[3], normal[3], uv[2];
};
struct CreatureMesh
{
    std::vector<CreatureVertex> vertices;
    std::vector<std::uint32_t> indices;
};

// Reads the little-endian NXM1 asset format and rejects malformed or oversized data.
std::optional<CreatureMesh> readCreatureMesh(std::istream& input);

struct AnimatedVertex
{
    float position[3], normal[3];
};
struct AnimatedMesh
{
    std::vector<std::array<float, 2>> uv;
    std::vector<std::uint32_t> indices;
    std::vector<std::vector<AnimatedVertex>> frames;
    float duration = 0.f;
};

// NXA1 stores a shared topology and baked skeletal poses. All sizes and
// floating-point fields are validated before a mesh reaches the renderer.
std::optional<AnimatedMesh> readAnimatedMesh(std::istream& input);
