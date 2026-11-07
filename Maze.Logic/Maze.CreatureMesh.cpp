#include "Maze.CreatureMesh.h"
#include <cmath>
#include <cstring>
#include <istream>
#include <limits>

static_assert(sizeof(CreatureVertex) == 8 * sizeof(float), "Unexpected vertex layout");

std::optional<CreatureMesh> readCreatureMesh(std::istream& input)
{
    char magic[4]{};
    std::uint32_t vertices = 0, indices = 0;
    if (!input.read(magic, sizeof(magic)) || std::memcmp(magic, "NXM1", 4) != 0 ||
        !input.read(reinterpret_cast<char*>(&vertices), sizeof(vertices)) ||
        !input.read(reinterpret_cast<char*>(&indices), sizeof(indices)) ||
        vertices == 0 || vertices > 200000 || indices == 0 || indices > 600000 || indices % 3 != 0)
        return std::nullopt;
    CreatureMesh mesh;
    mesh.vertices.resize(vertices); mesh.indices.resize(indices);
    if (!input.read(reinterpret_cast<char*>(mesh.vertices.data()), static_cast<std::streamsize>(mesh.vertices.size() * sizeof(CreatureVertex))) ||
        !input.read(reinterpret_cast<char*>(mesh.indices.data()), static_cast<std::streamsize>(mesh.indices.size() * sizeof(std::uint32_t))) ||
        input.peek() != std::char_traits<char>::eof()) return std::nullopt;
    for (const auto& vertex : mesh.vertices)
        for (float component : { vertex.position[0], vertex.position[1], vertex.position[2],
            vertex.normal[0], vertex.normal[1], vertex.normal[2], vertex.uv[0], vertex.uv[1] })
            if (!std::isfinite(component)) return std::nullopt;
    for (const auto index : mesh.indices) if (index >= vertices) return std::nullopt;
    return mesh;
}

std::optional<AnimatedMesh> readAnimatedMesh(std::istream& input)
{
    static_assert(sizeof(AnimatedVertex) == 6 * sizeof(float), "Unexpected animated vertex layout");
    char magic[4]{}; std::uint32_t vertices = 0, indices = 0, frames = 0; float duration = 0.f;
    if (!input.read(magic, sizeof(magic)) || std::memcmp(magic, "NXA1", 4) != 0 ||
        !input.read(reinterpret_cast<char*>(&vertices), sizeof(vertices)) ||
        !input.read(reinterpret_cast<char*>(&indices), sizeof(indices)) ||
        !input.read(reinterpret_cast<char*>(&frames), sizeof(frames)) ||
        !input.read(reinterpret_cast<char*>(&duration), sizeof(duration)) ||
        vertices == 0 || vertices > 100000 || indices == 0 || indices > 300000 || indices % 3 != 0 ||
        frames < 2 || frames > 60 || static_cast<std::uint64_t>(vertices) * frames > 2500000 ||
        !std::isfinite(duration) || duration <= 0.f || duration > 30.f) return std::nullopt;
    AnimatedMesh mesh; mesh.duration = duration;
    mesh.uv.resize(vertices); mesh.indices.resize(indices);
    if (!input.read(reinterpret_cast<char*>(mesh.uv.data()), static_cast<std::streamsize>(mesh.uv.size() * sizeof(mesh.uv[0]))) ||
        !input.read(reinterpret_cast<char*>(mesh.indices.data()), static_cast<std::streamsize>(mesh.indices.size() * sizeof(mesh.indices[0]))))
        return std::nullopt;
    for (const auto& uv : mesh.uv) for (float value : uv) if (!std::isfinite(value)) return std::nullopt;
    for (const auto index : mesh.indices) if (index >= vertices) return std::nullopt;
    mesh.frames.resize(frames);
    for (auto& frame : mesh.frames)
    {
        frame.resize(vertices);
        if (!input.read(reinterpret_cast<char*>(frame.data()), static_cast<std::streamsize>(frame.size() * sizeof(AnimatedVertex))))
            return std::nullopt;
        for (const auto& vertex : frame)
            for (float value : { vertex.position[0], vertex.position[1], vertex.position[2],
                vertex.normal[0], vertex.normal[1], vertex.normal[2] })
                if (!std::isfinite(value)) return std::nullopt;
    }
    if (input.peek() != std::char_traits<char>::eof()) return std::nullopt;
    return mesh;
}
