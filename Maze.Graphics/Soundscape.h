#pragma once

#include "Maze.Soundscape.h"
#include <memory>

// Owns a nonblocking audio worker. Missing playback devices leave the game silent.
class Soundscape
{
public:
    Soundscape();
    ~Soundscape();
    Soundscape(const Soundscape&) = delete;
    Soundscape& operator=(const Soundscape&) = delete;
    void setFrame(const SoundscapeFrame& frame) noexcept;
    bool muted() const noexcept;
    // 0 while opening, 1 when ready, -1 when no playback device could be opened.
    int status() const noexcept;
    void setMuted(bool muted) noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
