#pragma once

#include <SFML/Graphics.hpp>
#include <array>
#include <string>
#include <memory>
#include <utility>
#include <vector>
#include "GameState.h"
#include "Maze.Hunter.h"
#include "Maze.Soundscape.h"
#include "Maze.Puzzle.h"
#include "Maze.Safehouse.h"

struct GameInput
{
    float forward = 0.f;
    float strafe = 0.f;
    float turn = 0.f;
    bool run = false;
};

struct RemotePlayer
{
    float x = .5f, z = .5f, angle = 0.f;
    bool moving = false, running = false, present = false;
};

constexpr int maxCoopPlayers = 4;

class GameView
{
public:
    explicit GameView(const sf::Font& font);
    void resize(sf::Vector2u size);
    void reset(GameState* game = nullptr);
    void resetFeedback();
    void handleEvent(const sf::Event& event, GameState& game);
    void update(GameState& game, float seconds);
    void update(GameState& game, float seconds, const GameInput& input);
    void look(float horizontalPixels, float verticalPixels);
    void draw(sf::RenderTarget& target, const GameState& game);
    SoundscapeFrame soundFrame() const;
    void setSoundMuted(bool muted);
    void setSoundAvailability(int status) noexcept;
    void toggleHeadMotion();
    void setNetworkGuest(bool guest) noexcept { networkGuest_ = guest; if (!guest) networkSelfSlot_ = 0; }
    RemotePlayer localPlayer() const noexcept;
    void setRemotePlayer(int slot, const RemotePlayer& player) noexcept;
    std::string worldSnapshot() const;
    bool applyWorldSnapshot(GameState& game, const std::string& data, int selfSlot);
    void handleRemoteEvent(int slot, const sf::Event& event, GameState& game);
    void remoteDisconnected(int slot = -1) noexcept;
    bool initialized() const noexcept { return initialized_; }
    bool gateOpen() const noexcept { return gateOpen_; }
    int playerCount() const noexcept
    {
        int count = 1;
        for (const auto& player : remote_) if (player.present) ++count;
        return count;
    }
    void showNetworkNotice(const std::string& text) { notice_ = text; messageClock_ = 5.f; }
    void setNetworkStatus(std::string text) { networkStatusText_ = std::move(text); }

private:
    void initialize(GameState& game);
    bool walkable(const GameState& game, float x, float z) const;
    void interact(GameState& game);
    int nearbyShrine() const;
    bool nearGate(const GameState& game) const;
    bool nearInscription() const;
    void resetHunter(const GameState& game);
    void updateMonster(GameState& game, float seconds, bool playerMoving, bool playerRunning);
    void updateShift(GameState& game, float seconds);
    void invalidateChunksAround(MazePosition cell);
    void prepareChunks(const GameState& game);
    void updateChunkVisibility(const GameState& game);
    void drawGroundChunks(const GameState& game, int r0, int r1, int c0, int c1);
    void drawHedgeChunks(const GameState& game, int r0, int r1, int c0, int c1);
    void drawFoliageChunks(const GameState& game, int r0, int r1, int c0, int c1);
    sf::Vector2u viewport_{ 900u, 600u };
    sf::Text heading_, objective_, journal_, hint_, seedText_, feedback_, staminaLabel_, soundLabel_;
    bool initialized_ = false;
    std::unique_ptr<sf::Shader> worldShader_, creatureShader_, foliageShader_;
    bool shaderAttempted_ = false;
    unsigned int hedgeTexture_ = 0, groundTexture_ = 0, monsterTexture_ = 0, foliageTexture_ = 0;
    unsigned int shelterTexture_ = 0;
    unsigned int creatureMesh_ = 0, creatureTexture_ = 0;
    unsigned int survivorMesh_ = 0, survivorTexture_ = 0, survivorFrames_ = 0;
    float survivorDuration_ = 0.f;
    bool survivorMeshAttempted_ = false;
    bool creatureMeshAttempted_ = false;
    bool groundTextureAttempted_ = false, monsterTextureAttempted_ = false, foliageTextureAttempted_ = false;
    bool shelterTextureAttempted_ = false;
    std::vector<unsigned int> hedgeChunks_, groundChunks_, foliageChunks_;
    std::vector<bool> dirtyChunks_, visibleChunks_;
    int chunkRows_ = 0, chunkCols_ = 0;
    bool clearChunks_ = true;
    float x_ = .5f, z_ = .5f, angle_ = 0.f, pitch_ = 0.f, elapsed_ = 0.f;
    float inscriptionX_ = .10f, inscriptionZ_ = .75f;
    float ambientTime_ = 0.f, darkness_ = 0.f;
    float bobPhase_ = 0.f, bobBlend_ = 0.f, sprintBlend_ = 0.f;
    bool headMotionEnabled_ = true;
    std::uint32_t thunderEvent_ = 0;
    bool soundMoving_ = false, soundRunning_ = false, soundMuted_ = false;
    bool networkGuest_ = false;
    int networkSelfSlot_ = 0;
    std::array<RemotePlayer, maxCoopPlayers> remote_{}, remoteVisual_{};
    std::vector<MazePosition> safehouses_;
    std::array<int, maxCoopPlayers> checkpoints_{ { -1, -1, -1, -1 } };
    std::array<std::uint32_t, maxCoopPlayers> remoteCaptures_{};
    std::uint32_t seenRemoteCaptures_ = 0;
    std::array<float, maxCoopPlayers> remoteCaptureCooldown_{};
    int soundStatus_ = 0;
    float monsterClock_ = 0.f, monsterStepDuration_ = 1.15f, shiftClock_ = 0.f, messageClock_ = 0.f;
    float directorClock_ = 0.f;
    std::uint32_t directorEvent_ = 0;
    float monsterHeading_ = 0.f, monsterTargetHeading_ = 0.f, creatureGait_ = 0.f;
    enum class ShiftPhase { Idle, Opening, Open, Closing };
    ShiftPhase shiftPhase_ = ShiftPhase::Idle;
    float shiftProgress_ = 0.f;
    std::uint32_t shiftCount_ = 0, captureCount_ = 0;
    float stamina_ = 1.f;
    bool exhausted_ = false;
    bool movementHeard_ = false, runningHeard_ = false;
    HunterState hunter_{};
    MazePosition monster_{}, monsterFrom_{}, shift_{ -1, -1 };
    MazePuzzle puzzle_{};
    std::array<MazePosition, 3> shrines_{};
    std::array<int, 3> order_{};
    std::array<int, 3> clues_{}, gateCode_{};
    std::array<bool, 3> discoveredClues_{};
    GateRule gateRule_ = GateRule::Forward;
    std::string gateInput_;
    bool gateOpen_ = false, inscriptionRead_ = false;
    int solved_ = 0;
    unsigned activeStones_ = 0;
    std::string notice_;
    std::string networkStatusText_;
};
