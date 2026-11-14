#pragma once
#include "VoicePacket.h"
#include <memory>
#include <vector>

// Capture starts only while explicitly held push-to-talk is enabled. Audio is
// transient memory only; stopping/disconnecting discards capture/play queues.
class VoiceChat
{
public:
    VoiceChat();
    ~VoiceChat();
    VoiceChat(const VoiceChat&)=delete;
    VoiceChat& operator=(const VoiceChat&)=delete;
    void setTalking(bool held);
    void setDevicesEnabled(bool enabled);
    void setMuted(bool muted);
    void play(const VoicePacket& packet,float gain,float pan,float clarity);
    std::vector<VoicePacket> captured();
    void reset();
    int status() const noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
