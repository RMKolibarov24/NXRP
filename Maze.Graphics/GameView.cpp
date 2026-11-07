#include "GameView.h"
#include "Maze.Puzzle.h"
#include "Maze.Atmosphere.h"
#include "Maze.CreatureMesh.h"
#include "ExecutablePath.h"
#include <SFML/OpenGL.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <random>
#include <sstream>
#include <vector>

namespace
{
    constexpr float pi = 3.14159265f;
    const char* names[] = { "MOON", "THORN", "EYE" };
    const float colors[][3] = { { .42f, .68f, .85f }, { .80f, .35f, .25f }, { .76f, .69f, .32f } };
    bool same(MazePosition a, MazePosition b) { return a.row == b.row && a.col == b.col; }
    bool open(const MazeGrid& maze, int row, int col)
    {
        return maze.contains({ row, col }) && maze.at(row, col) != MazeCell::Wall;
    }
    void quad(float ax, float ay, float az, float bx, float by, float bz,
              float cx, float cy, float cz, float dx, float dy, float dz,
              float nx, float ny, float nz)
    {
        glNormal3f(nx, ny, nz);
        glTexCoord2f(0, 0); glVertex3f(ax, ay, az);
        glTexCoord2f(1, 0); glVertex3f(bx, by, bz);
        glTexCoord2f(1, 1); glVertex3f(cx, cy, cz);
        glTexCoord2f(0, 1); glVertex3f(dx, dy, dz);
    }
    void subdividedQuad(float ax, float ay, float az, float bx, float by, float bz,
                         float cx, float cy, float cz, float dx, float dy, float dz,
                         float nx, float ny, float nz, int divisions)
    {
        glNormal3f(nx, ny, nz);
        const auto vertex = [&](float u, float v)
        {
            const float x = (ax + (bx-ax)*u)*(1-v) + (dx + (cx-dx)*u)*v;
            const float y = (ay + (by-ay)*u)*(1-v) + (dy + (cy-dy)*u)*v;
            const float z = (az + (bz-az)*u)*(1-v) + (dz + (cz-dz)*u)*v;
            glTexCoord2f(u, v); glVertex3f(x, y, z);
        };
        for (int y = 0; y < divisions; ++y) for (int x = 0; x < divisions; ++x)
        {
            const float u0 = static_cast<float>(x) / divisions, u1 = static_cast<float>(x+1) / divisions;
            const float v0 = static_cast<float>(y) / divisions, v1 = static_cast<float>(y+1) / divisions;
            vertex(u0,v0); vertex(u1,v0); vertex(u1,v1); vertex(u0,v1);
        }
    }
    void emitBox(float x, float z, float half, float height)
    {
        const float l = x-half, r = x+half, n = z-half, f = z+half;
        quad(l,0,n,r,0,n,r,height,n,l,height,n,0,0,-1);
        quad(r,0,f,l,0,f,l,height,f,r,height,f,0,0,1);
        quad(l,0,f,l,0,n,l,height,n,l,height,f,-1,0,0);
        quad(r,0,n,r,0,f,r,height,f,r,height,n,1,0,0);
        quad(l,height,n,r,height,n,r,height,f,l,height,f,0,1,0);
    }
    void box(float x, float z, float half, float height)
    {
        glBegin(GL_QUADS); emitBox(x, z, half, height); glEnd();
    }
    void drawNightSky(float x, float y, float z, float flash, float time, std::uint32_t seed)
    {
        constexpr float radius = 20.f;
        constexpr int sectors = 32, bands = 8;
        glDisable(GL_DEPTH_TEST); glDepthMask(GL_FALSE);
        glDisable(GL_LIGHTING); glDisable(GL_FOG); glDisable(GL_TEXTURE_2D);
        glPushMatrix(); glTranslatef(x, y, z);
        glBegin(GL_QUADS);
        for (int band = 0; band < bands; ++band) for (int sector = 0; sector < sectors; ++sector)
        {
            const auto vertex = [&](int b, int s)
            {
                const float elevation = (pi * .5f) * static_cast<float>(b) / bands;
                const float azimuth = 2.f * pi * static_cast<float>(s) / sectors;
                const float cloud = .5f + .5f * std::sin(azimuth * 3.f + std::sin(elevation * 4.f)
                    + static_cast<float>(seed % 97u) * .07f + time * .015f);
                const float horizon = 1.f - static_cast<float>(b) / bands;
                const float glow = .004f + .011f * cloud * horizon + flash * (.10f + .06f * cloud);
                glColor3f(.012f + glow * .65f, .019f + glow * .8f, .030f + glow);
                const float ring = radius * std::cos(elevation);
                glVertex3f(ring * std::sin(azimuth), radius * std::sin(elevation), ring * std::cos(azimuth));
            };
            vertex(band,sector); vertex(band,sector+1); vertex(band+1,sector+1); vertex(band+1,sector);
        }
        glEnd();
        // Sparse stars stay fixed in world space and fade during a flash.
        glPointSize(1.f); glBegin(GL_POINTS);
        for (unsigned i = 0; i < 40; ++i)
        {
            unsigned h = seed ^ (i * 0x9e3779b9u); h ^= h >> 16; h *= 0x7feb352du; h ^= h >> 15;
            const float azimuth = (h & 0xffffu) / 65535.f * 2.f * pi;
            const float elevation = .30f + ((h >> 16) & 0xffffu) / 65535.f * 1.15f;
            const float brightness = (.15f + (h % 11u) * .006f) * (1.f - flash);
            glColor3f(brightness * .8f, brightness * .9f, brightness);
            const float ring = radius * .99f * std::cos(elevation);
            glVertex3f(ring * std::sin(azimuth), radius * .99f * std::sin(elevation), ring * std::cos(azimuth));
        }
        glEnd(); glPopMatrix(); glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING); glEnable(GL_FOG);
    }

    void foliageCard(float x, float z, float nx, float nz, float along, float y,
                     float width, float height, float slant)
    {
        const float tx = -nz, tz = nx;
        const float centerX = x + tx * along + nx * .06f;
        const float centerZ = z + tz * along + nz * .06f;
        const float hx = tx * width * .5f + nx * slant;
        const float hz = tz * width * .5f + nz * slant;
        quad(centerX-hx, y, centerZ-hz, centerX+hx, y, centerZ+hz,
             centerX+hx, y+height, centerZ+hz, centerX-hx, y+height, centerZ-hz, nx,0,nz);
    }
    void part(float x, float y, float z, float width, float height, float depth)
    {
        glPushMatrix(); glTranslatef(x, y, z); glScalef(width, height, depth);
        box(0.f, 0.f, .5f, 1.f); glPopMatrix();
    }
    void ellipsoid(float x, float y, float z, float rx, float ry, float rz)
    {
        constexpr int slices = 12, bands = 8;
        glPushMatrix(); glTranslatef(x, y, z); glScalef(rx, ry, rz);
        for (int band = 0; band < bands; ++band)
        {
            const float lower = -pi * .5f + pi * static_cast<float>(band) / bands;
            const float upper = -pi * .5f + pi * static_cast<float>(band + 1) / bands;
            glBegin(GL_QUAD_STRIP);
            for (int slice = 0; slice <= slices; ++slice)
            {
                const float azimuth = 2.f * pi * static_cast<float>(slice) / slices;
                for (const float elevation : { lower, upper })
                {
                    const float nx = std::cos(elevation) * std::cos(azimuth);
                    const float ny = std::sin(elevation);
                    const float nz = std::cos(elevation) * std::sin(azimuth);
                    glNormal3f(nx, ny, nz); glVertex3f(nx, ny, nz);
                }
            }
            glEnd();
        }
        glPopMatrix();
    }
    void shrineGlyph(int symbol, int number)
    {
        glLineWidth(2.f);
        // Symbol and numeral float just above the stone and face the viewer.
        glBegin(GL_LINES);
        if (symbol == 0)
        {
            for (int i = 0; i < 16; ++i)
            {
                const float a = -pi * .65f + i * pi * 1.3f / 16.f;
                const float b = -pi * .65f + (i + 1) * pi * 1.3f / 16.f;
                glVertex3f(.10f * std::cos(a), .16f + .10f * std::sin(a), 0);
                glVertex3f(.10f * std::cos(b), .16f + .10f * std::sin(b), 0);
            }
        }
        else if (symbol == 1)
        {
            glVertex3f(0,.05f,0); glVertex3f(0,.27f,0);
            glVertex3f(-.10f,.11f,0); glVertex3f(.10f,.22f,0);
            glVertex3f(-.10f,.22f,0); glVertex3f(.10f,.11f,0);
        }
        else
        {
            glVertex3f(-.12f,.16f,0); glVertex3f(0,.24f,0);
            glVertex3f(0,.24f,0); glVertex3f(.12f,.16f,0);
            glVertex3f(.12f,.16f,0); glVertex3f(0,.08f,0);
            glVertex3f(0,.08f,0); glVertex3f(-.12f,.16f,0);
            glVertex3f(0,.12f,0); glVertex3f(0,.20f,0);
        }
        constexpr float segments[][4] = {
            {-.07f,.00f,.07f,.00f}, {.07f,.00f,.07f,-.10f},
            {.07f,-.10f,.07f,-.20f}, {-.07f,-.20f,.07f,-.20f},
            {-.07f,-.10f,-.07f,-.20f}, {-.07f,.00f,-.07f,-.10f},
            {-.07f,-.10f,.07f,-.10f} };
        constexpr unsigned masks[] = { 0, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f };
        for (int i = 0; i < 7; ++i) if (masks[number] & (1u << i))
        {
            glVertex3f(segments[i][0], segments[i][1], 0);
            glVertex3f(segments[i][2], segments[i][3], 0);
        }
        glEnd(); glLineWidth(1.f);
    }
    void stoneCap(float x, float z, float half, float base, float peak)
    {
        glBegin(GL_TRIANGLES);
        glNormal3f(0.f, .45f, -1.f);
        glVertex3f(x-half,base,z-half); glVertex3f(x+half,base,z-half); glVertex3f(x,peak,z);
        glNormal3f(1.f, .45f, 0.f);
        glVertex3f(x+half,base,z-half); glVertex3f(x+half,base,z+half); glVertex3f(x,peak,z);
        glNormal3f(0.f, .45f, 1.f);
        glVertex3f(x+half,base,z+half); glVertex3f(x-half,base,z+half); glVertex3f(x,peak,z);
        glNormal3f(-1.f, .45f, 0.f);
        glVertex3f(x-half,base,z+half); glVertex3f(x-half,base,z-half); glVertex3f(x,peak,z);
        glEnd();
    }
    unsigned int loadCreatureMesh(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        auto mesh = readCreatureMesh(file);
        if (!mesh) return 0;
        const auto list = glGenLists(1);
        if (!list) return 0;
        glNewList(list, GL_COMPILE); glBegin(GL_TRIANGLES);
        for (auto index : mesh->indices)
        {
            const auto& vertex = mesh->vertices[index];
            glNormal3fv(vertex.normal); glTexCoord2fv(vertex.uv); glVertex3fv(vertex.position);
        }
        glEnd(); glEndList();
        return list;
    }

    struct UploadedAnimation { unsigned int first = 0, frames = 0; float duration = 0.f; };
    UploadedAnimation loadAnimatedMesh(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        const auto mesh = readAnimatedMesh(file);
        if (!mesh) return {};
        const auto count = static_cast<unsigned int>(mesh->frames.size());
        const auto first = glGenLists(static_cast<GLsizei>(count));
        if (!first) return {};
        for (unsigned int frame = 0; frame < count; ++frame)
        {
            glNewList(first + frame, GL_COMPILE); glBegin(GL_TRIANGLES);
            for (const auto index : mesh->indices)
            {
                const auto& vertex = mesh->frames[frame][index];
                glNormal3fv(vertex.normal); glTexCoord2fv(mesh->uv[index].data()); glVertex3fv(vertex.position);
            }
            glEnd(); glEndList();
        }
        return { first, count, mesh->duration };
    }

    unsigned int loadSurfaceTexture(const std::filesystem::path& path, unsigned int side = 512)
    {
        // Normalize the source to a power-of-two texture for older OpenGL drivers.
        sf::Image source;
        if (source.loadFromFile(path))
        {
            const auto size = source.getSize();
            const auto* input = source.getPixelsPtr();
            std::vector<unsigned char> rgba(side * side * 4);
            for (unsigned int y = 0; y < side; ++y) for (unsigned int x = 0; x < side; ++x)
            {
                const auto from = ((y * size.y / side) * size.x + x * size.x / side) * 4;
                const auto to = (y * side + x) * 4;
                for (int channel = 0; channel < 4; ++channel) rgba[to + channel] = input[from + channel];
            }
            unsigned int texture = 0;
            glGenTextures(1, &texture); glBindTexture(GL_TEXTURE_2D, texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, side, side, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
            return texture;
        }
        return 0;
    }
    unsigned int makeHedgeTexture()
    {
        if (const auto texture = loadSurfaceTexture(executableDirectory() / "assets" / "textures" / "hedge.png")) return texture;
        // Keep the maze readable if the optional texture is missing.
        constexpr int side = 128;
        std::vector<unsigned char> pixels(side * side * 3);
        std::mt19937 rng(0x64726162u);
        for (int y = 0; y < side; ++y) for (int x = 0; x < side; ++x)
        {
            const auto offset = (y * side + x) * 3;
            const auto noise = static_cast<unsigned char>(rng() % 13);
            pixels[offset] = 18 + noise / 2; pixels[offset + 1] = 32 + noise;
            pixels[offset + 2] = 16 + noise / 2;
        }
        // Layer small, wrapped ellipses so the procedural foliage tiles at cell edges.
        for (int leaf = 0; leaf < 800; ++leaf)
        {
            const int cx = rng() % side, cy = rng() % side;
            const int rx = 2 + rng() % 5, ry = 3 + rng() % 7;
            const int light = rng() % 48;
            for (int dy = -ry; dy <= ry; ++dy) for (int dx = -rx; dx <= rx; ++dx)
            {
                const float d = float(dx * dx) / (rx * rx) + float(dy * dy) / (ry * ry);
                if (d > 1.f) continue;
                const int x = (cx + dx + side) % side, y = (cy + dy + side) % side;
                const auto offset = (y * side + x) * 3;
                const int edge = d > .7f ? -10 : 0;
                const int vein = dx == 0 ? 9 : 0;
                pixels[offset] = static_cast<unsigned char>(std::max(0, 34 + light / 2 + edge + vein));
                pixels[offset + 1] = static_cast<unsigned char>(std::max(0, 64 + light + edge + vein));
                pixels[offset + 2] = static_cast<unsigned char>(std::max(0, 26 + light / 2 + edge));
            }
        }
        unsigned int texture = 0;
        glGenTextures(1, &texture); glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, side, side, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        return texture;
    }

}

GameView::GameView(const sf::Font& font)
    : heading_(font, "THE HEDGE", 24), objective_(font, "", 19), journal_(font, "", 16), hint_(font, "", 17),
      seedText_(font, "", 15), feedback_(font, "", 20), staminaLabel_(font, "BREATH", 14), soundLabel_(font, "", 14)
{
    heading_.setFillColor({ 231, 226, 198 });
    objective_.setFillColor({ 226, 222, 198 });
    journal_.setFillColor({ 177, 185, 165 });
    hint_.setFillColor({ 177, 185, 165 });
    seedText_.setFillColor({ 144, 155, 139 });
    feedback_.setFillColor({ 237, 189, 126 });
    staminaLabel_.setFillColor({ 177, 185, 165 });
    soundLabel_.setFillColor({ 177, 185, 165 });
}
void GameView::resize(sf::Vector2u size) { if (size.x && size.y) viewport_ = size; }
void GameView::reset(GameState* game)
{
    if (game && initialized_ && shift_.row >= 0 && game->maze.contains(shift_))
        game->maze.at(shift_.row, shift_.col) = MazeCell::Wall;
    initialized_ = false; clearChunks_ = true; notice_.clear();
}
void GameView::resetFeedback() { }

void GameView::initialize(GameState& game)
{
    x_ = z_ = .5f;
    const bool south = open(game.maze, 1, 0);
    angle_ = south ? 0.f : pi / 2.f; pitch_ = 0.f;
    inscriptionX_ = south ? .10f : .75f;
    inscriptionZ_ = south ? .75f : .10f;
    elapsed_ = monsterClock_ = shiftClock_ = ambientTime_ = directorClock_ = 0.f;
    directorEvent_ = 0;
    thunderEvent_ = 0; soundMoving_ = soundRunning_ = false;
    bobPhase_ = bobBlend_ = sprintBlend_ = 0.f;
    darkness_ = hedgeDarkness(x_, z_, game.mazeSeed.value_or(0));
    shift_ = { -1, -1 }; shiftPhase_ = ShiftPhase::Idle; shiftProgress_ = 0.f; shiftCount_ = 0; captureCount_ = 0;
    remoteCaptures_.fill(0); seenRemoteCaptures_ = 0; remoteCaptureCooldown_.fill(0.f);
    safehouses_ = generateSafehouses(game.maze.size(), game.mazeSeed.value_or(0));
    checkpoints_.fill(-1);
    solved_ = 0; activeStones_ = 0; gateOpen_ = false; inscriptionRead_ = false; gateInput_.clear();
    discoveredClues_.fill(false);
    puzzle_ = generateMazePuzzle(game.maze, game.mazeSeed.value_or(0));
    shrines_ = puzzle_.stones; order_ = puzzle_.order; clues_ = puzzle_.numbers;
    gateRule_ = puzzle_.gateRule; gateCode_ = gateCode(puzzle_);
    resetHunter(game);
    monsterStepDuration_ = 1.15f; stamina_ = 1.f; exhausted_ = false;
    monsterHeading_ = monsterTargetHeading_ = 0.f; creatureGait_ = 0.f;
    movementHeard_ = runningHeard_ = false;
    notice_ = "An inscription waits near the entrance. Press E to read it.";
    messageClock_ = 0.f;
    initialized_ = true;
}

bool GameView::walkable(const GameState& game, float x, float z) const
{
    constexpr float radius = .17f;
    for (float dx : { -radius, radius }) for (float dz : { -radius, radius })
    {
        const int row = static_cast<int>(std::floor(z + dz));
        const int col = static_cast<int>(std::floor(x + dx));
        if (!open(game.maze, row, col) || (!gateOpen_ && game.maze.at(row, col) == MazeCell::Exit)) return false;
    }
    // Cabins have a doorway in the middle of each side. Their walls are
    // physical, while the generator guarantees clear approaches to the doors.
    for (const auto house : safehouses_)
    {
        const float hx = house.col + .5f, hz = house.row + .5f;
        for (float side : { -1.f, 1.f }) for (float end : { -1.f, 1.f })
        {
            if (std::abs(x - (hx + side * 1.45f)) < .05f + radius &&
                std::abs(z - (hz + end * 1.02f)) < .43f + radius) return false;
            if (std::abs(z - (hz + side * 1.45f)) < .05f + radius &&
                std::abs(x - (hx + end * 1.02f)) < .43f + radius) return false;
        }
    }
    return true;
}

void GameView::handleEvent(const sf::Event& event, GameState& game)
{
    if (!initialized_ || game.phase != GamePhase::Playing) return;
    const auto* key = event.getIf<sf::Event::KeyPressed>();
    if (!key) return;
    if (key->code == sf::Keyboard::Key::E) { interact(game); return; }
    if (!nearGate(game) || solved_ != 3 || gateOpen_) return;
    int digit = 0;
    switch (key->code)
    {
    case sf::Keyboard::Key::Num1: case sf::Keyboard::Key::Numpad1: digit = 1; break;
    case sf::Keyboard::Key::Num2: case sf::Keyboard::Key::Numpad2: digit = 2; break;
    case sf::Keyboard::Key::Num3: case sf::Keyboard::Key::Numpad3: digit = 3; break;
    case sf::Keyboard::Key::Num4: case sf::Keyboard::Key::Numpad4: digit = 4; break;
    case sf::Keyboard::Key::Num5: case sf::Keyboard::Key::Numpad5: digit = 5; break;
    case sf::Keyboard::Key::Num6: case sf::Keyboard::Key::Numpad6: digit = 6; break;
    case sf::Keyboard::Key::Num7: case sf::Keyboard::Key::Numpad7: digit = 7; break;
    case sf::Keyboard::Key::Num8: case sf::Keyboard::Key::Numpad8: digit = 8; break;
    case sf::Keyboard::Key::Num9: case sf::Keyboard::Key::Numpad9: digit = 9; break;
    case sf::Keyboard::Key::Backspace: if (!gateInput_.empty()) gateInput_.pop_back(); return;
    case sf::Keyboard::Key::Enter:
        if (gateInput_.size() == 3)
        {
            std::string answer;
            for (int number : gateCode_) answer += static_cast<char>('0' + number);
            if (gateInput_ == answer)
            {
                gateOpen_ = true; notice_ = "The gate opens. Step through.";
                if (game.maze.at(game.player.row, game.player.col) == MazeCell::Exit) game.phase = GamePhase::Won;
            }
            else notice_ = "The lock rejects the code. Study its inscription.";
            gateInput_.clear(); messageClock_ = 4.f;
        }
        return;
    default: return;
    }
    if (gateInput_.size() < 3) gateInput_ += static_cast<char>('0' + digit);
}

int GameView::nearbyShrine() const
{
    int nearest = -1;
    float best = .55f * .55f;
    for (int i = 0; i < 3; ++i)
    {
        const float dx = x_ - (shrines_[i].col + .5f), dz = z_ - (shrines_[i].row + .5f);
        const float distance = dx * dx + dz * dz;
        if (distance < best) { best = distance; nearest = i; }
    }
    return nearest;
}

bool GameView::nearGate(const GameState& game) const
{
    const auto& size = game.maze.size();
    const float dx = x_ - (size.cols - .5f), dz = z_ - (size.rows - .5f);
    return dx * dx + dz * dz < 1.15f * 1.15f;
}

bool GameView::nearInscription() const
{
    const float dx = x_ - inscriptionX_, dz = z_ - inscriptionZ_;
    return dx * dx + dz * dz < .7f * .7f;
}

void GameView::interact(GameState& game)
{
    const int i = nearbyShrine();
    if (i >= 0)
    {
        const auto result = touchStone(puzzle_, i, solved_, activeStones_);
        if (result == StoneTouchResult::Solved || result == StoneTouchResult::AlreadySolved)
            notice_ = "The seal is broken. Read the stone numbers at the gate.";
        else if (result == StoneTouchResult::Reset) notice_ = "The stones fall silent. The sequence begins again.";
        else if (puzzle_.stoneKind == StonePuzzleKind::Balance)
            notice_ = "The lit stones total " + std::to_string(stoneBalance(puzzle_, activeStones_)) + ".";
        else notice_ = "The stone answers. Find the next symbol.";
        messageClock_ = 4.f;
        return;
    }
    if (nearInscription())
    {
        inscriptionRead_ = true;
        notice_ = puzzle_.stoneKind == StonePuzzleKind::Balance
            ? "Light stones to match the total; E toggles each stone."
            : "You record the inscription's sequence.";
        messageClock_ = 4.f;
        return;
    }
    if (nearGate(game))
    { notice_ = solved_ == 3 ? "The gate inscription explains the code." : "The gate is sealed by three stones."; messageClock_ = 3.f; }
}

void GameView::resetHunter(const GameState& game)
{
    hunter_ = makeHunter(game.maze, game.mazeSeed.value_or(0), captureCount_, shrines_, shift_, safehouses_);
    monster_ = hunter_.cell; monsterFrom_ = monster_;
}

RemotePlayer GameView::localPlayer() const noexcept
{
    return { x_, z_, angle_, soundMoving_, soundRunning_, initialized_ };
}

void GameView::setRemotePlayer(int slot, const RemotePlayer& player) noexcept
{
    if (slot < 0 || slot >= maxCoopPlayers) return;
    auto& current = remote_[slot]; auto& visual = remoteVisual_[slot];
    const float visualDx = player.x - visual.x, visualDz = player.z - visual.z;
    if (!current.present || visualDx * visualDx + visualDz * visualDz > 9.f) visual = player;
    current = player;
    if (!initialized_ || !player.present) return;
    if (!networkGuest_ && slot > 0)
    {
        const auto cell = MazePosition{ static_cast<int>(player.z), static_cast<int>(player.x) };
        const int house = safehouseAt(cell, safehouses_);
        if (house >= 0) checkpoints_[slot] = house;
    }
    for (int i = 0; i < 3; ++i)
    {
        const float dx = player.x - (shrines_[i].col + .5f), dz = player.z - (shrines_[i].row + .5f);
        if (dx * dx + dz * dz < .55f * .55f) discoveredClues_[i] = true;
    }
}

void GameView::remoteDisconnected(int slot) noexcept
{
    if (slot < 0)
    {
        for (int i = 0; i < maxCoopPlayers; ++i) remoteDisconnected(i);
        return;
    }
    if (slot >= maxCoopPlayers) return;
    remote_[slot].present = remoteVisual_[slot].present = false;
    remoteCaptures_[slot] = 0; remoteCaptureCooldown_[slot] = 0.f; checkpoints_[slot] = -1;
}

std::string GameView::worldSnapshot() const
{
    unsigned clues = 0;
    for (unsigned i = 0; i < discoveredClues_.size(); ++i) if (discoveredClues_[i]) clues |= 1u << i;
    std::ostringstream out;
    out.precision(8);
    out << x_ << ' ' << z_ << ' ' << angle_ << ' ' << soundMoving_ << ' ' << soundRunning_;
    for (int slot = 1; slot < maxCoopPlayers; ++slot)
    {
        const auto& player = remote_[slot];
        out << ' ' << player.present << ' ' << player.x << ' ' << player.z << ' ' << player.angle
            << ' ' << player.moving << ' ' << player.running;
    }
    out << ' '
        << monsterFrom_.row << ' ' << monsterFrom_.col << ' ' << monster_.row << ' ' << monster_.col << ' '
        << monsterClock_ << ' ' << monsterStepDuration_ << ' ' << monsterHeading_ << ' ' << creatureGait_ << ' '
        << elapsed_ << ' ' << ambientTime_ << ' ' << static_cast<int>(hunter_.mode) << ' ' << static_cast<int>(shiftPhase_) << ' '
        << shift_.row << ' ' << shift_.col << ' ' << shiftProgress_ << ' '
        << solved_ << ' ' << activeStones_ << ' ' << inscriptionRead_ << ' ' << gateOpen_ << ' '
        << (gateInput_.empty() ? "-" : gateInput_) << ' ' << clues;
    for (int slot = 1; slot < maxCoopPlayers; ++slot) out << ' ' << remoteCaptures_[slot];
    for (int slot = 0; slot < maxCoopPlayers; ++slot) out << ' ' << checkpoints_[slot];
    return out.str();
}

bool GameView::applyWorldSnapshot(GameState& game, const std::string& data, int selfSlot)
{
    if (!initialized_ || game.maze.empty() || selfSlot < 1 || selfSlot >= maxCoopPlayers) return false;
    std::istringstream in(data);
    std::array<RemotePlayer, maxCoopPlayers> players{};
    std::array<int, maxCoopPlayers> presence{}, moving{}, running{};
    std::array<std::uint32_t, maxCoopPlayers> captures{};
    std::array<int, maxCoopPlayers> checkpoints{};
    MazePosition from{}, monster{}, shift{};
    float clock, duration, heading, gait, elapsed, ambient, progress;
    int hunterMode, phase, solved, inscription, gate;
    unsigned clues, activeStones;
    std::string input, extra;
    presence[0] = 1;
    if (!(in >> players[0].x >> players[0].z >> players[0].angle >> moving[0] >> running[0])) return false;
    for (int slot = 1; slot < maxCoopPlayers; ++slot)
        if (!(in >> presence[slot] >> players[slot].x >> players[slot].z >> players[slot].angle >> moving[slot] >> running[slot])) return false;
    if (!(in >> from.row >> from.col >> monster.row >> monster.col >> clock >> duration >> heading >> gait
        >> elapsed >> ambient >> hunterMode >> phase >> shift.row >> shift.col >> progress >> solved >> activeStones >> inscription >> gate
        >> input >> clues)) return false;
    for (int slot = 1; slot < maxCoopPlayers; ++slot) if (!(in >> captures[slot])) return false;
    for (int slot = 0; slot < maxCoopPlayers; ++slot) if (!(in >> checkpoints[slot])) return false;
    if (in >> extra) return false;
    const auto finite = [](float value) { return std::isfinite(value); };
    const auto size = game.maze.size();
    const auto validCell = [&](MazePosition p) { return game.maze.contains(p); };
    for (int slot = 0; slot < maxCoopPlayers; ++slot)
    {
        const auto& player = players[slot];
        if (!finite(player.x) || !finite(player.z) || !finite(player.angle) || player.x < 0 || player.z < 0 ||
            player.x >= size.cols || player.z >= size.rows || (presence[slot] != 0 && presence[slot] != 1) ||
            (moving[slot] != 0 && moving[slot] != 1) || (running[slot] != 0 && running[slot] != 1) ||
            checkpoints[slot] < -1 || checkpoints[slot] >= static_cast<int>(safehouses_.size())) return false;
    }
    if (!validCell(from) || !validCell(monster) ||
        !finite(clock) || !finite(duration) || !finite(heading) || !finite(gait) || !finite(elapsed) ||
        !finite(ambient) || !finite(progress) || clock < 0 || duration <= 0 || duration > 5 || elapsed < 0 || ambient < 0 ||
        progress < 0 || progress > 1 || hunterMode < 0 || hunterMode > 2 || phase < 0 || phase > 3 || solved < 0 || solved > 3 || clues > 7 || activeStones > 7 ||
        (inscription != 0 && inscription != 1) || (gate != 0 && gate != 1) ||
        (phase == 0 ? (shift.row != -1 || shift.col != -1) : !validCell(shift)) ||
        (input != "-" && (input.size() > 3 || input.find_first_not_of("123456789") != std::string::npos))) return false;

    const auto oldShift = shift_;
    const auto oldPhase = shiftPhase_;
    if (oldPhase == ShiftPhase::Open && game.maze.contains(oldShift))
        game.maze.at(oldShift.row, oldShift.col) = MazeCell::Wall;
    shift_ = shift; shiftPhase_ = static_cast<ShiftPhase>(phase); shiftProgress_ = progress;
    if (shiftPhase_ == ShiftPhase::Open) game.maze.at(shift.row, shift.col) = MazeCell::Passage;
    if (!same(oldShift, shift_) || oldPhase != shiftPhase_)
    {
        if (game.maze.contains(oldShift)) invalidateChunksAround(oldShift);
        if (game.maze.contains(shift_)) invalidateChunksAround(shift_);
    }
    for (int slot = 0; slot < maxCoopPlayers; ++slot)
    {
        if (slot == selfSlot || !presence[slot]) { remoteDisconnected(slot); continue; }
        players[slot].moving = moving[slot] != 0; players[slot].running = running[slot] != 0; players[slot].present = true;
        setRemotePlayer(slot, players[slot]);
    }
    checkpoints_ = checkpoints; networkSelfSlot_ = selfSlot;
    monsterFrom_ = from; monster_ = monster; monsterClock_ = clock; monsterStepDuration_ = duration;
    monsterHeading_ = heading; creatureGait_ = gait; elapsed_ = elapsed; ambientTime_ = ambient;
    hunter_.mode = static_cast<HunterMode>(hunterMode);
    solved_ = solved; activeStones_ = activeStones; inscriptionRead_ = inscription != 0; gateOpen_ = gate != 0;
    gateInput_ = input == "-" ? "" : input;
    for (unsigned i = 0; i < discoveredClues_.size(); ++i) discoveredClues_[i] = (clues & (1u << i)) != 0;
    if (captures[selfSlot] != seenRemoteCaptures_)
    {
        seenRemoteCaptures_ = captures[selfSlot];
        const int house = checkpoints_[selfSlot];
        const MazePosition respawn = house >= 0 ? safehouses_[house] : MazePosition{ 0, 0 };
        x_ = respawn.col + .5f; z_ = respawn.row + .5f;
        angle_ = open(game.maze, respawn.row + 1, respawn.col) ? 0.f : pi / 2.f; pitch_ = 0.f;
        game.player = respawn; stamina_ = 1.f; exhausted_ = false;
        notice_ = house >= 0 ? "It caught you. You wake in the safehouse." : "It caught you. You wake again at the entrance.";
        messageClock_ = 5.f;
    }
    return true;
}

void GameView::handleRemoteEvent(int slot, const sf::Event& event, GameState& game)
{
    if (slot < 0 || slot >= maxCoopPlayers || !remote_[slot].present) return;
    const float savedX = x_, savedZ = z_;
    x_ = remote_[slot].x; z_ = remote_[slot].z;
    const int stone = nearbyShrine();
    if (stone >= 0) discoveredClues_[stone] = true;
    handleEvent(event, game);
    x_ = savedX; z_ = savedZ;
}

void GameView::updateMonster(GameState& game, float seconds, bool playerMoving, bool playerRunning)
{
    if (elapsed_ < 12.f) { monsterClock_ = 0.f; movementHeard_ = runningHeard_ = false; return; }
    directorClock_ += seconds;
    if (directorClock_ >= 55.f)
    {
        directorClock_ = 0.f;
        std::array<HunterTarget, maxCoopPlayers> group{};
        std::size_t count = 0; group[count++] = { game.player, playerMoving, playerRunning };
        for (int slot = 1; slot < maxCoopPlayers; ++slot)
            if (remote_[slot].present) group[count++] = { { static_cast<int>(remote_[slot].z), static_cast<int>(remote_[slot].x) },
                remote_[slot].moving, remote_[slot].running };
        if (relocateDistantHunter(game.maze, hunter_, group.data(), count, shrines_, safehouses_,
            game.mazeSeed.value_or(0), ++directorEvent_))
        {
            monster_ = monsterFrom_ = hunter_.cell; monsterClock_ = 0.f;
            monsterHeading_ = monsterTargetHeading_ = 0.f;
        }
    }
    for (auto& cooldown : remoteCaptureCooldown_) cooldown = std::max(0.f, cooldown - seconds);
    movementHeard_ = movementHeard_ || playerMoving;
    runningHeard_ = runningHeard_ || playerRunning;
    monsterClock_ += seconds;
    if (monsterClock_ >= monsterStepDuration_)
    {
        monsterClock_ -= monsterStepDuration_;
        monsterFrom_ = monster_;
        const auto previousMode = hunter_.mode;
        std::array<HunterTarget, maxCoopPlayers> targets{};
        std::size_t count = 0;
        targets[count++] = { game.player, movementHeard_, runningHeard_ };
        for (int slot = 1; slot < maxCoopPlayers; ++slot)
        {
            const auto& player = remote_[slot];
            if (player.present) targets[count++] = { { static_cast<int>(player.z), static_cast<int>(player.x) },
                player.moving, player.running };
        }
        advanceHunterGroup(game.maze, hunter_, targets.data(), count, safehouses_);
        movementHeard_ = runningHeard_ = false;
        monster_ = hunter_.cell;
        if (!same(monster_, monsterFrom_))
            monsterTargetHeading_ = std::atan2(static_cast<float>(monster_.col - monsterFrom_.col),
                static_cast<float>(monster_.row - monsterFrom_.row));
        monsterStepDuration_ = hunter_.mode == HunterMode::Chase ? .84f : 1.15f;
        if (hunter_.mode == HunterMode::Chase && previousMode != HunterMode::Chase)
        {
            notice_ = "It has found you."; messageClock_ = 3.f;
        }
    }
    const float turn = std::remainder(monsterTargetHeading_ - monsterHeading_, 2.f * pi);
    monsterHeading_ += std::clamp(turn, -seconds * 4.f, seconds * 4.f);
    if (!same(monster_, monsterFrom_)) creatureGait_ += seconds * (hunter_.mode == HunterMode::Chase ? 9.f : 6.f);
    const float blend = std::clamp(monsterClock_ / monsterStepDuration_, 0.f, 1.f);
    const float mx = monsterFrom_.col + .5f + (monster_.col - monsterFrom_.col) * blend;
    const float mz = monsterFrom_.row + .5f + (monster_.row - monsterFrom_.row) * blend;
    const float dx = x_ - mx, dz = z_ - mz;
    for (int slot = 1; slot < maxCoopPlayers; ++slot)
    {
        auto& player = remote_[slot];
        const float remoteDx = player.x - mx, remoteDz = player.z - mz;
        const MazePosition playerCell{ static_cast<int>(player.z), static_cast<int>(player.x) };
        if (player.present && safehouseAt(playerCell, safehouses_) < 0 && remoteCaptureCooldown_[slot] <= 0.f &&
            remoteDx * remoteDx + remoteDz * remoteDz < .38f * .38f)
        {
            ++remoteCaptures_[slot];
            const int house = checkpoints_[slot];
            const MazePosition respawn = house >= 0 ? safehouses_[house] : MazePosition{ 0, 0 };
            player.x = respawn.col + .5f; player.z = respawn.row + .5f; remoteCaptureCooldown_[slot] = 2.f;
            notice_ = "A companion was caught."; messageClock_ = 4.f;
        }
    }
    if (safehouseAt(game.player, safehouses_) < 0 && dx * dx + dz * dz < .38f * .38f)
    {
        const int house = checkpoints_[0];
        const MazePosition respawn = house >= 0 ? safehouses_[house] : MazePosition{ 0, 0 };
        x_ = respawn.col + .5f; z_ = respawn.row + .5f;
        angle_ = open(game.maze, respawn.row + 1, respawn.col) ? 0.f : pi / 2.f; pitch_ = 0.f;
        game.player = respawn; gateInput_.clear();
        ++captureCount_; resetHunter(game);
        elapsed_ = monsterClock_ = directorClock_ = 0.f; monsterStepDuration_ = 1.15f;
        monsterHeading_ = monsterTargetHeading_ = 0.f; creatureGait_ = 0.f;
        stamina_ = 1.f; exhausted_ = false; movementHeard_ = runningHeard_ = false;
        bobBlend_ = sprintBlend_ = 0.f;
        notice_ = house >= 0 ? "It caught you. You wake in the safehouse." : "It caught you. You wake again at the entrance.";
        messageClock_ = 5.f;
    }
}

void GameView::look(float horizontalPixels, float verticalPixels)
{
    if (!initialized_) return;
    angle_ -= std::clamp(horizontalPixels, -120.f, 120.f) * .0025f;
    pitch_ = std::clamp(pitch_ - std::clamp(verticalPixels, -120.f, 120.f) * .0025f, -1.f, 1.f);
}

void GameView::update(GameState& game, float seconds)
{
    const auto down = [](sf::Keyboard::Key key) { return sf::Keyboard::isKeyPressed(key); };
    GameInput input;
    input.turn = (down(sf::Keyboard::Key::Right) ? 1.f : 0.f) - (down(sf::Keyboard::Key::Left) ? 1.f : 0.f);
    input.forward = (down(sf::Keyboard::Key::W) || down(sf::Keyboard::Key::Up) ? 1.f : 0.f)
        - (down(sf::Keyboard::Key::S) || down(sf::Keyboard::Key::Down) ? 1.f : 0.f);
    input.strafe = (down(sf::Keyboard::Key::D) ? 1.f : 0.f) - (down(sf::Keyboard::Key::A) ? 1.f : 0.f);
    input.run = down(sf::Keyboard::Key::LShift);
    update(game, seconds, input);
}

void GameView::update(GameState& game, float seconds, const GameInput& input)
{
    if (game.phase != GamePhase::Playing || game.maze.empty()) return;
    if (!initialized_) initialize(game);
    seconds = std::clamp(seconds, 0.f, .05f);
    for (int slot = 0; slot < maxCoopPlayers; ++slot)
    {
        const auto& player = remote_[slot]; auto& visual = remoteVisual_[slot];
        if (!player.present) continue;
        const float blend = 1.f - std::exp(-12.f * seconds);
        visual.x += (player.x - visual.x) * blend;
        visual.z += (player.z - visual.z) * blend;
        visual.angle += std::remainder(player.angle - visual.angle, 2.f * pi) * blend;
        visual.moving = player.moving; visual.running = player.running;
    }
    elapsed_ += seconds; messageClock_ = std::max(0.f, messageClock_ - seconds);
    const auto previousStorm = sampleStorm(ambientTime_, game.mazeSeed.value_or(0)).event;
    ambientTime_ += seconds;
    const auto currentStorm = sampleStorm(ambientTime_, game.mazeSeed.value_or(0)).event;
    if (currentStorm != previousStorm) ++thunderEvent_;
    angle_ -= std::clamp(input.turn, -1.f, 1.f) * 1.8f * seconds;
    const float forward = std::clamp(input.forward, -1.f, 1.f);
    const float strafe = std::clamp(input.strafe, -1.f, 1.f);
    const float length = std::sqrt(forward * forward + strafe * strafe);
    const bool running = length > 0.f && input.run && !exhausted_;
    const float oldX = x_, oldZ = z_;
    if (length > 0.f)
    {
        const float speed = (running ? 2.15f : 1.45f) * seconds / length;
        const float dx = (std::sin(angle_) * forward - std::cos(angle_) * strafe) * speed;
        const float dz = (std::cos(angle_) * forward + std::sin(angle_) * strafe) * speed;
        if (walkable(game, x_ + dx, z_)) x_ += dx;
        if (walkable(game, x_, z_ + dz)) z_ += dz;
        MazePosition cell{ static_cast<int>(z_), static_cast<int>(x_) };
        if (!same(cell, game.player))
        {
            game.player = cell;
            if (game.statistics.successfulMoves < std::numeric_limits<std::uint64_t>::max()) ++game.statistics.successfulMoves;
            if (game.maze.at(cell.row, cell.col) == MazeCell::Exit && gateOpen_) { game.phase = GamePhase::Won; return; }
        }
    }
    const bool moved = std::abs(x_ - oldX) + std::abs(z_ - oldZ) > .00001f;
    const int currentHouse = safehouseAt(game.player, safehouses_);
    if (currentHouse >= 0 && checkpoints_[networkSelfSlot_] != currentHouse)
    {
        checkpoints_[networkSelfSlot_] = currentHouse;
        notice_ = "Safehouse found. The creature cannot enter."; messageClock_ = 5.f;
    }
    // A nearby stone displays its engraving, so record it for the gate puzzle.
    // The journal survives a capture but is cleared when the maze is restarted.
    const int observedStone = nearbyShrine();
    if (observedStone >= 0) discoveredClues_[observedStone] = true;
    if (running && moved)
    {
        stamina_ = std::max(0.f, stamina_ - seconds * .16f);
        if (stamina_ <= 0.f) exhausted_ = true;
    }
    else
    {
        stamina_ = std::min(1.f, stamina_ + seconds * (currentHouse >= 0 ? .22f : .10f));
        if (stamina_ >= .3f) exhausted_ = false;
    }
    const float motionTarget = moved && headMotionEnabled_ ? 1.f : 0.f;
    bobBlend_ += (motionTarget - bobBlend_) * std::min(1.f, seconds * 6.f);
    const float sprintTarget = running && moved && headMotionEnabled_ ? 1.f : 0.f;
    sprintBlend_ += (sprintTarget - sprintBlend_) * std::min(1.f, seconds * 4.f);
    if (moved) bobPhase_ += seconds * (running ? 10.f : 7.f);
    const float targetDarkness = hedgeDarkness(x_, z_, game.mazeSeed.value_or(0));
    darkness_ += (targetDarkness - darkness_) * std::min(1.f, seconds * 1.5f);
    soundMoving_ = moved; soundRunning_ = running && moved;
    if (networkGuest_)
    {
        monsterClock_ = std::min(monsterClock_ + seconds, monsterStepDuration_);
        if (shiftPhase_ == ShiftPhase::Opening || shiftPhase_ == ShiftPhase::Closing)
            shiftProgress_ = std::min(1.f, shiftProgress_ + seconds / 2.f);
    }
    else
    {
        updateMonster(game, seconds, moved, running && moved);
        updateShift(game, seconds);
    }
}

void GameView::updateShift(GameState& game, float seconds)
{
    constexpr float transitionSeconds = 2.f;
    if (shiftPhase_ == ShiftPhase::Opening || shiftPhase_ == ShiftPhase::Closing)
    {
        shiftProgress_ = std::min(1.f, shiftProgress_ + seconds / transitionSeconds);
        if (shiftProgress_ < 1.f) return;
        if (shiftPhase_ == ShiftPhase::Opening)
        {
            game.maze.at(shift_.row, shift_.col) = MazeCell::Passage;
            shiftPhase_ = ShiftPhase::Open; shiftClock_ = 0.f;
            invalidateChunksAround(shift_);
        }
        else
        {
            const auto finished = shift_;
            shift_ = { -1, -1 }; shiftPhase_ = ShiftPhase::Idle; shiftClock_ = 0.f;
            invalidateChunksAround(finished);
        }
        return;
    }
    shiftClock_ += seconds;
    if (shiftClock_ < 24.f) return;
    if (shiftPhase_ == ShiftPhase::Open)
    {
        const bool playerOverlaps = x_ + .17f > shift_.col && x_ - .17f < shift_.col + 1.f
            && z_ + .17f > shift_.row && z_ - .17f < shift_.row + 1.f;
        bool remoteOverlaps = false;
        for (const auto& player : remote_)
            remoteOverlaps = remoteOverlaps || (player.present && player.x + .17f > shift_.col && player.x - .17f < shift_.col + 1.f
                && player.z + .17f > shift_.row && player.z - .17f < shift_.row + 1.f);
        if (playerOverlaps || remoteOverlaps || same(shift_, game.player) || same(shift_, monster_) || same(shift_, monsterFrom_)) return;
        game.maze.at(shift_.row, shift_.col) = MazeCell::Wall;
        shiftPhase_ = ShiftPhase::Closing; shiftProgress_ = 0.f;
        invalidateChunksAround(shift_);
        notice_ = "The hedge is closing."; messageClock_ = 3.f;
        return;
    }
    std::vector<MazePosition> candidates, nearbyCandidates;
    const auto size = game.maze.size();
    for (int row = 1; row < size.rows - 1; ++row) for (int col = 1; col < size.cols - 1; ++col)
        if (game.maze.at(row, col) == MazeCell::Wall &&
            ((open(game.maze,row-1,col) && open(game.maze,row+1,col)) ||
             (open(game.maze,row,col-1) && open(game.maze,row,col+1))))
        {
            const MazePosition cell{ row, col };
            candidates.push_back(cell);
            int distance = std::abs(row - game.player.row) + std::abs(col - game.player.col);
            for (const auto& player : remote_) if (player.present)
                distance = std::min(distance, std::abs(row - static_cast<int>(player.z))
                    + std::abs(col - static_cast<int>(player.x)));
            // Make changes perceptible, but never begin one directly beside a player.
            if (distance >= 3 && distance <= 12) nearbyCandidates.push_back(cell);
        }
    if (candidates.empty()) { shiftClock_ = 0.f; return; }
    const auto& choices = nearbyCandidates.empty() ? candidates : nearbyCandidates;
    const auto index = (game.mazeSeed.value_or(0) + ++shiftCount_ * 2654435761u) % choices.size();
    shift_ = choices[index]; shiftPhase_ = ShiftPhase::Opening; shiftProgress_ = 0.f;
    invalidateChunksAround(shift_);
    notice_ = "Somewhere in the hedge, branches move."; messageClock_ = 3.f;
}

void GameView::invalidateChunksAround(MazePosition cell)
{
    constexpr int chunkSize = 8;
    for (int row = cell.row - 1; row <= cell.row + 1; ++row)
        for (int col = cell.col - 1; col <= cell.col + 1; ++col)
        {
            if (row < 0 || col < 0) continue;
            const int cr = row / chunkSize, cc = col / chunkSize;
            if (cr < chunkRows_ && cc < chunkCols_)
                dirtyChunks_[static_cast<std::size_t>(cr * chunkCols_ + cc)] = true;
        }
}

void GameView::prepareChunks(const GameState& game)
{
    constexpr int chunkSize = 8;
    const auto size = game.maze.size();
    const int rows = (size.rows + chunkSize - 1) / chunkSize;
    const int cols = (size.cols + chunkSize - 1) / chunkSize;
    if (!clearChunks_ && rows == chunkRows_ && cols == chunkCols_) return;
    for (const auto list : hedgeChunks_) if (list) glDeleteLists(list, 1);
    for (const auto list : groundChunks_) if (list) glDeleteLists(list, 1);
    for (const auto list : foliageChunks_) if (list) glDeleteLists(list, 1);
    chunkRows_ = rows; chunkCols_ = cols;
    hedgeChunks_.assign(static_cast<std::size_t>(rows * cols), 0);
    groundChunks_.assign(hedgeChunks_.size(), 0);
    foliageChunks_.assign(hedgeChunks_.size(), 0);
    dirtyChunks_.assign(hedgeChunks_.size(), true);
    visibleChunks_.assign(hedgeChunks_.size(), true);
    clearChunks_ = false;
}

void GameView::updateChunkVisibility(const GameState& game)
{
    prepareChunks(game);
    GLfloat projection[16], modelView[16], clip[16]{};
    glGetFloatv(GL_PROJECTION_MATRIX, projection);
    glGetFloatv(GL_MODELVIEW_MATRIX, modelView);
    // OpenGL matrices are column-major; clip = projection * model-view.
    for (int col = 0; col < 4; ++col) for (int row = 0; row < 4; ++row)
        for (int k = 0; k < 4; ++k)
            clip[col * 4 + row] += projection[k * 4 + row] * modelView[col * 4 + k];
    float planes[6][4]{};
    for (int component = 0; component < 4; ++component)
    {
        const int offset = component * 4;
        for (int axis = 0; axis < 3; ++axis)
        {
            planes[axis * 2][component] = clip[offset + 3] + clip[offset + axis];
            planes[axis * 2 + 1][component] = clip[offset + 3] - clip[offset + axis];
        }
    }
    const auto size = game.maze.size();
    constexpr int chunkSize = 8;
    // The extra margin includes boundary hedge boxes and wind-blown foliage.
    constexpr float margin = 1.25f;
    for (int cr = 0; cr < chunkRows_; ++cr) for (int cc = 0; cc < chunkCols_; ++cc)
    {
        const float minX = cc * chunkSize - margin;
        const float maxX = std::min(size.cols, (cc + 1) * chunkSize) + margin;
        const float minZ = cr * chunkSize - margin;
        const float maxZ = std::min(size.rows, (cr + 1) * chunkSize) + margin;
        bool visible = true;
        for (const auto& p : planes)
        {
            const float x = p[0] >= 0.f ? maxX : minX;
            const float y = p[1] >= 0.f ? 2.5f : -.1f;
            const float z = p[2] >= 0.f ? maxZ : minZ;
            if (p[0] * x + p[1] * y + p[2] * z + p[3] < 0.f)
            { visible = false; break; }
        }
        visibleChunks_[static_cast<std::size_t>(cr * chunkCols_ + cc)] = visible;
    }
}

void GameView::drawGroundChunks(const GameState& game, int r0, int r1, int c0, int c1)
{
    constexpr int chunkSize = 8;
    prepareChunks(game);
    const auto size = game.maze.size();
    for (int cr = r0 / chunkSize; cr <= r1 / chunkSize; ++cr)
        for (int cc = c0 / chunkSize; cc <= c1 / chunkSize; ++cc)
        {
            const auto index = static_cast<std::size_t>(cr * chunkCols_ + cc);
            if (!visibleChunks_[index]) continue;
            if (!groundChunks_[index])
            {
                groundChunks_[index] = glGenLists(1);
                if (!groundChunks_[index]) continue;
                glNewList(groundChunks_[index], GL_COMPILE);
                glBegin(GL_QUADS);
                const int lastRow = std::min(size.rows, (cr + 1) * chunkSize);
                const int lastCol = std::min(size.cols, (cc + 1) * chunkSize);
                for (int row = cr * chunkSize; row < lastRow; ++row)
                    for (int col = cc * chunkSize; col < lastCol; ++col)
                    {
                        const unsigned grain = (static_cast<unsigned>(row) * 92821u ^ static_cast<unsigned>(col) * 68917u) % 15u;
                        const float shade = grain / 255.f;
                        if (groundTexture_) glColor3f(.85f + shade, .85f + shade, .82f + shade);
                        else glColor3f(.19f + shade, .19f + shade, .14f + shade * .7f);
                        subdividedQuad(static_cast<float>(col),0,static_cast<float>(row),static_cast<float>(col+1),0,static_cast<float>(row),
                            static_cast<float>(col+1),0,static_cast<float>(row+1),static_cast<float>(col),0,static_cast<float>(row+1),0,1,0,2);
                    }
                glEnd(); glEndList();
            }
            glCallList(groundChunks_[index]);
        }
}

void GameView::drawHedgeChunks(const GameState& game, int r0, int r1, int c0, int c1)
{
    constexpr int chunkSize = 8;
    prepareChunks(game);
    const auto size = game.maze.size();
    const int cols = chunkCols_;
    if (!hedgeTexture_) hedgeTexture_ = makeHedgeTexture();
    if (!foliageTextureAttempted_)
    {
        foliageTexture_ = loadSurfaceTexture(executableDirectory() / "assets" / "textures" / "foliage.png");
        foliageTextureAttempted_ = true;
    }
    glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, hedgeTexture_);
    for (int cr = r0 / chunkSize; cr <= r1 / chunkSize; ++cr)
        for (int cc = c0 / chunkSize; cc <= c1 / chunkSize; ++cc)
        {
            const auto index = static_cast<std::size_t>(cr * cols + cc);
            if (!visibleChunks_[index]) continue;
            if (!hedgeChunks_[index]) hedgeChunks_[index] = glGenLists(1);
            if (!hedgeChunks_[index]) continue;
            if (dirtyChunks_[index])
            {
                glNewList(hedgeChunks_[index], GL_COMPILE);
                glBegin(GL_QUADS);
                const int lastRow = std::min(size.rows, (cr + 1) * chunkSize);
                const int lastCol = std::min(size.cols, (cc + 1) * chunkSize);
                for (int row = cr * chunkSize; row < lastRow; ++row)
                    for (int col = cc * chunkSize; col < lastCol; ++col)
                    {
                        const unsigned variation = static_cast<unsigned>(row) * 73856093u
                            ^ static_cast<unsigned>(col) * 19349663u ^ game.mazeSeed.value_or(0);
                        const float shade = static_cast<float>(variation % 24) / 255.f;
                        glColor3f(.75f + shade, .75f + shade, .70f + shade);
                        const bool movingWall = same({row, col}, shift_) &&
                            (shiftPhase_ == ShiftPhase::Opening || shiftPhase_ == ShiftPhase::Closing);
                        const auto visiblePassage = [&](int r, int c)
                        {
                            return open(game.maze, r, c) || (same({r, c}, shift_) &&
                                (shiftPhase_ == ShiftPhase::Opening || shiftPhase_ == ShiftPhase::Closing));
                        };
                        if (game.maze.at(row, col) == MazeCell::Wall && !movingWall)
                        {
                            const float l = static_cast<float>(col), r = l + 1.f;
                            const float n = static_cast<float>(row), f = n + 1.f;
                            const float h = 1.8f + static_cast<float>(variation % 5) * .035f;
                            if (visiblePassage(row - 1, col)) subdividedQuad(l,0,n,r,0,n,r,h,n,l,h,n,0,0,-1,4);
                            if (visiblePassage(row + 1, col)) subdividedQuad(r,0,f,l,0,f,l,h,f,r,h,f,0,0,1,4);
                            if (visiblePassage(row, col - 1)) subdividedQuad(l,0,f,l,0,n,l,h,n,l,h,f,-1,0,0,4);
                            if (visiblePassage(row, col + 1)) subdividedQuad(r,0,n,r,0,f,r,h,f,r,h,n,1,0,0,4);
                            quad(l,h,n,r,h,n,r,h,f,l,h,f,0,1,0);
                            glColor3f(.85f + shade, .85f + shade, .80f + shade);
                            quad(l+.12f,1.95f,n+.12f,r-.12f,1.95f,n+.12f,
                                 r-.12f,1.95f,f-.12f,l+.12f,1.95f,f-.12f,0,1,0);
                        }
                        else if (!movingWall)
                        {
                            // Enclose passages at the grid edge without changing collision data.
                            if (row == 0) emitBox(col + .5f, -.5f, .5f, 1.9f);
                            if (row == size.rows - 1) emitBox(col + .5f, size.rows + .5f, .5f, 1.9f);
                            if (col == 0) emitBox(-.5f, row + .5f, .5f, 1.9f);
                            if (col == size.cols - 1) emitBox(size.cols + .5f, row + .5f, .5f, 1.9f);
                        }
                    }
                glEnd();
                glEndList();
                if (foliageTexture_ && !foliageChunks_[index]) foliageChunks_[index] = glGenLists(1);
                if (foliageChunks_[index])
                {
                    glNewList(foliageChunks_[index], GL_COMPILE); glBegin(GL_QUADS);
                    for (int row = cr * chunkSize; row < lastRow; ++row)
                        for (int col = cc * chunkSize; col < lastCol; ++col)
                        {
                            if (game.maze.at(row, col) != MazeCell::Wall ||
                                (same({row, col}, shift_) && (shiftPhase_ == ShiftPhase::Opening || shiftPhase_ == ShiftPhase::Closing))) continue;
                            const unsigned variation = static_cast<unsigned>(row) * 73856093u
                                ^ static_cast<unsigned>(col) * 19349663u ^ game.mazeSeed.value_or(0);
                            const float shade = static_cast<float>(variation % 20u) / 255.f;
                            glColor3f(.78f + shade, .80f + shade, .74f + shade);
                            const auto exposed = [&](int r, int c)
                            {
                                return open(game.maze, r, c) || (same({r,c}, shift_) &&
                                    (shiftPhase_ == ShiftPhase::Opening || shiftPhase_ == ShiftPhase::Closing));
                            };
                            const auto face = [&](float x, float z, float nx, float nz, unsigned salt)
                            {
                                for (int n = 0; n < 4; ++n)
                                {
                                    const unsigned h = variation ^ (salt * 0x9e3779b9u) ^ (static_cast<unsigned>(n) * 0x85ebca6bu);
                                    const float along = (n % 2 == 0 ? -.23f : .23f) + static_cast<float>(h % 7u) * .01f;
                                    const float y = .12f + n * .51f + static_cast<float>((h >> 4) % 5u) * .015f;
                                    const float slant = (h & 1u) ? .10f : -.10f;
                                    foliageCard(x, z, nx, nz, along, y, .55f, .48f, slant);
                                }
                            };
                            if (exposed(row-1,col)) face(col+.5f, static_cast<float>(row), 0.f,-1.f,1u);
                            if (exposed(row+1,col)) face(col+.5f, row+1.f, 0.f,1.f,2u);
                            if (exposed(row,col-1)) face(static_cast<float>(col), row+.5f, -1.f,0.f,3u);
                            if (exposed(row,col+1)) face(col+1.f, row+.5f, 1.f,0.f,4u);
                        }
                    glEnd(); glEndList();
                }
                dirtyChunks_[index] = false;
            }
            glCallList(hedgeChunks_[index]);
        }
    glDisable(GL_TEXTURE_2D);
}

void GameView::drawFoliageChunks(const GameState& game, int r0, int r1, int c0, int c1)
{
    if (!foliageTexture_) return;
    constexpr int chunkSize = 8;
    prepareChunks(game);
    glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, foliageTexture_);
    glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, .4f);
    for (int cr = r0 / chunkSize; cr <= r1 / chunkSize; ++cr)
        for (int cc = c0 / chunkSize; cc <= c1 / chunkSize; ++cc)
        {
            const auto index = static_cast<std::size_t>(cr * chunkCols_ + cc);
            if (!visibleChunks_[index]) continue;
            const auto list = foliageChunks_[index];
            if (list) glCallList(list);
        }
    glDisable(GL_ALPHA_TEST); glDisable(GL_TEXTURE_2D);
}

SoundscapeFrame GameView::soundFrame() const
{
    SoundscapeFrame frame;
    frame.active = initialized_; frame.moving = soundMoving_; frame.running = soundRunning_;
    frame.thunderEvent = thunderEvent_;
    if (elapsed_ >= 12.f)
    {
        const float blend = std::clamp(monsterClock_ / monsterStepDuration_, 0.f, 1.f);
        const float mx = monsterFrom_.col + .5f + (monster_.col - monsterFrom_.col) * blend;
        const float mz = monsterFrom_.row + .5f + (monster_.row - monsterFrom_.row) * blend;
        const float dx = mx - x_, dz = mz - z_;
        const float distance = std::sqrt(dx*dx + dz*dz);
        frame.threat = std::clamp(1.f - distance / 11.f, 0.f, 1.f);
        if (distance > .001f) frame.hunterPan = std::clamp((-dx * std::cos(angle_) + dz * std::sin(angle_)) / distance, -1.f, 1.f);
        frame.hunterMoving = !same(monster_, monsterFrom_);
        frame.hunterChasing = hunter_.mode == HunterMode::Chase;
        frame.hunterStepSeconds = monsterStepDuration_;
    }
    if (shiftPhase_ == ShiftPhase::Opening || shiftPhase_ == ShiftPhase::Closing)
    {
        const float dx = shift_.col + .5f - x_, dz = shift_.row + .5f - z_;
        const float distance = std::sqrt(dx*dx + dz*dz);
        frame.shifting = std::clamp(1.f - distance / 8.f, 0.f, 1.f);
        if (distance > .001f) frame.shiftingPan = std::clamp((-dx * std::cos(angle_) + dz * std::sin(angle_)) / distance, -1.f, 1.f);
    }
    return frame;
}

void GameView::setSoundAvailability(int status) noexcept { soundStatus_ = status; }

void GameView::toggleHeadMotion()
{
    headMotionEnabled_ = !headMotionEnabled_;
    if (!headMotionEnabled_) bobBlend_ = sprintBlend_ = 0.f;
    notice_ = headMotionEnabled_ ? "Head motion on." : "Head motion off.";
    messageClock_ = 2.f;
}

void GameView::setSoundMuted(bool muted)
{
    soundMuted_ = muted;
    notice_ = muted ? "Sound muted." : "Sound on.";
    messageClock_ = 2.f;
}

void GameView::draw(sf::RenderTarget& target, const GameState& game)
{
    if (!initialized_ || game.maze.empty()) return;
    if (!shaderAttempted_)
    {
        shaderAttempted_ = true;
        if (sf::Shader::isAvailable())
        {
            auto shader = std::make_unique<sf::Shader>();
            const auto shaders = executableDirectory() / "assets" / "shaders";
            if (shader->loadFromFile(shaders / "world.vert", shaders / "world.frag"))
            {
                shader->setUniform("surface", sf::Shader::CurrentTexture);
                worldShader_ = std::move(shader);
            }
            auto foliage = std::make_unique<sf::Shader>();
            if (foliage->loadFromFile(shaders / "foliage.vert", shaders / "world.frag"))
            {
                foliage->setUniform("surface", sf::Shader::CurrentTexture);
                foliageShader_ = std::move(foliage);
            }
            auto creature = std::make_unique<sf::Shader>();
            if (creature->loadFromFile(shaders / "creature.vert", shaders / "world.frag"))
            {
                creature->setUniform("surface", sf::Shader::CurrentTexture);
                creatureShader_ = std::move(creature);
            }
        }
    }
    target.pushGLStates();
    glViewport(0, 0, static_cast<GLsizei>(viewport_.x), static_cast<GLsizei>(viewport_.y));
    glDepthMask(GL_TRUE); glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D); glDisable(GL_BLEND); glEnable(GL_NORMALIZE);
    // Prepare the creature on the first gameplay frame, before a chase can expose a load hitch.
    if (!creatureMeshAttempted_)
    {
        const auto models = executableDirectory() / "assets" / "models";
        creatureMesh_ = loadCreatureMesh(models / "stalker.nxm");
        if (creatureMesh_) creatureTexture_ = loadSurfaceTexture(models / "stalker.jpg");
        creatureMeshAttempted_ = true;
    }
    if (!survivorMeshAttempted_ && !networkStatusText_.empty())
    {
        const auto models = executableDirectory() / "assets" / "models";
        const auto mesh = loadAnimatedMesh(models / "survivor.nxa");
        survivorMesh_ = mesh.first; survivorFrames_ = mesh.frames; survivorDuration_ = mesh.duration;
        if (survivorMesh_) survivorTexture_ = loadSurfaceTexture(models / "survivor.png", 1024);
        survivorMeshAttempted_ = true;
    }
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    const float eyeHeight = .72f + .009f * bobBlend_ * std::sin(bobPhase_ * 2.f);
    const float roll = .35f * bobBlend_ * std::sin(bobPhase_);
    const double nearPlane = .05, top = nearPlane * .65 * (1.0 + .035 * sprintBlend_);
    const double right = top * static_cast<double>(viewport_.x) / viewport_.y;
    glFrustum(-right, right, -top, top, nearPlane, 24.0);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glRotatef(-roll, 0.f, 0.f, 1.f);
    glRotatef(-pitch_ * 180.f / pi, 1.f, 0.f, 0.f);
    glRotatef(180.f - angle_ * 180.f / pi, 0.f, 1.f, 0.f);
    glTranslatef(-x_, -eyeHeight, -z_);
    glEnable(GL_FOG); glFogi(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, .12f + .075f * darkness_ + .006f * std::sin(ambientTime_ * .17f));
    glEnable(GL_LIGHTING); glEnable(GL_LIGHT0); glEnable(GL_LIGHT1); glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    const auto storm = sampleStorm(ambientTime_, game.mazeSeed.value_or(0));
    const float flash = storm.flash;
    const GLfloat fog[] = { .027f + flash * .10f, .038f + flash * .12f, .043f + flash * .16f, 1.f };
    glFogfv(GL_FOG_COLOR, fog);
    glClearColor(fog[0], fog[1], fog[2], 1.f); glClear(GL_COLOR_BUFFER_BIT);
    const float ambientScale = 1.f - darkness_ * .45f;
    const GLfloat ambient[] = { .10f * ambientScale + flash * .16f,
        .13f * ambientScale + flash * .18f, .12f * ambientScale + flash * .22f, 1.f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
    const float threat = soundFrame().threat;
    const float interference = std::pow(std::max(0.f, std::sin(ambientTime_ * 7.7f + std::sin(ambientTime_ * 1.3f))), 6.f);
    const float flicker = (.82f + .12f * std::sin(ambientTime_ * 5.1f) + .06f * std::sin(ambientTime_ * 13.7f))
        * (1.f - .32f * threat * interference);
    const GLfloat lamp[] = { 1.f * flicker, .82f * flicker, .55f * flicker, 1.f };
    const GLfloat lampPos[] = { x_, eyeHeight, z_, 1.f };
    const GLfloat lampDir[] = { std::sin(angle_) * std::cos(pitch_), std::sin(pitch_), std::cos(angle_) * std::cos(pitch_) };
    glLightfv(GL_LIGHT0, GL_POSITION, lampPos); glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, lampDir);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lamp); glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 52.f - darkness_ * 9.f);
    glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 3.f); glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, .10f);
    const GLfloat moon[] = { .17f + flash * .55f, .24f + flash * .58f, .28f + flash * .62f, 1.f };
    const GLfloat moonPos[] = { -1.f, 3.f, -1.f, 0.f };
    glLightfv(GL_LIGHT1, GL_POSITION, moonPos); glLightfv(GL_LIGHT1, GL_DIFFUSE, moon);
    int nearestHouse = -1; float nearestHouseDistance = 64.f;
    for (std::size_t i = 0; i < safehouses_.size(); ++i)
    {
        const float dx = safehouses_[i].col + .5f - x_, dz = safehouses_[i].row + .5f - z_;
        const float distance = dx * dx + dz * dz;
        if (distance < nearestHouseDistance) { nearestHouse = static_cast<int>(i); nearestHouseDistance = distance; }
    }
    const float lanternFlicker = .88f + .08f * std::sin(ambientTime_ * 4.1f) + .04f * std::sin(ambientTime_ * 9.7f);
    const GLfloat shelterLight[] = { nearestHouse >= 0 ? .85f * lanternFlicker : 0.f,
        nearestHouse >= 0 ? .53f * lanternFlicker : 0.f, nearestHouse >= 0 ? .24f * lanternFlicker : 0.f, 1.f };
    const GLfloat shelterPosition[] = { nearestHouse >= 0 ? safehouses_[nearestHouse].col + .5f : 0.f,
        1.5f, nearestHouse >= 0 ? safehouses_[nearestHouse].row + .5f : 0.f, 1.f };
    glEnable(GL_LIGHT2); glLightfv(GL_LIGHT2, GL_POSITION, shelterPosition); glLightfv(GL_LIGHT2, GL_DIFFUSE, shelterLight);
    glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, .28f); glLightf(GL_LIGHT2, GL_QUADRATIC_ATTENUATION, .13f);
    drawNightSky(x_, eyeHeight, z_, flash, ambientTime_, game.mazeSeed.value_or(0));
    updateChunkVisibility(game);
    const auto size = game.maze.size();
    const int r0 = std::max(0, static_cast<int>(z_) - 10), r1 = std::min(size.rows - 1, static_cast<int>(z_) + 10);
    const int c0 = std::max(0, static_cast<int>(x_) - 10), c1 = std::min(size.cols - 1, static_cast<int>(x_) + 10);
    // Subdivide the ground so the moving flashlight is evaluated throughout the path.
    if (!groundTextureAttempted_)
    {
        groundTexture_ = loadSurfaceTexture(executableDirectory() / "assets" / "textures" / "ground.png");
        groundTextureAttempted_ = true;
    }
    if (worldShader_)
    {
        worldShader_->setUniform("darkness", darkness_);
        worldShader_->setUniform("flicker", flicker);
        worldShader_->setUniform("flash", flash);
        worldShader_->setUniform("textured", groundTexture_ != 0);
        worldShader_->setUniform("wetness", .20f);
        sf::Shader::bind(worldShader_.get());
    }
    if (groundTexture_) { glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, groundTexture_); }
    drawGroundChunks(game, r0, r1, c0, c1);
    glDisable(GL_TEXTURE_2D);
    if (worldShader_)
    {
        worldShader_->setUniform("textured", true);
        worldShader_->setUniform("wetness", .06f);
    }
    drawHedgeChunks(game, r0, r1, c0, c1);
    if (foliageShader_)
    {
        foliageShader_->setUniform("darkness", darkness_);
        foliageShader_->setUniform("flicker", flicker);
        foliageShader_->setUniform("flash", flash);
        foliageShader_->setUniform("textured", true);
        foliageShader_->setUniform("wetness", .10f);
        foliageShader_->setUniform("time", ambientTime_);
        sf::Shader::bind(foliageShader_.get());
    }
    drawFoliageChunks(game, std::max(0, static_cast<int>(z_) - 6), std::min(size.rows - 1, static_cast<int>(z_) + 6),
        std::max(0, static_cast<int>(x_) - 6), std::min(size.cols - 1, static_cast<int>(x_) + 6));
    if (worldShader_ || foliageShader_) sf::Shader::bind(nullptr);
    if (shiftPhase_ == ShiftPhase::Opening || shiftPhase_ == ShiftPhase::Closing)
    {
        const float height = 1.9f * (shiftPhase_ == ShiftPhase::Opening ? 1.f - shiftProgress_ : shiftProgress_);
        if (height > .001f)
        {
            glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, hedgeTexture_);
            glColor3f(.8f, .8f, .75f); box(shift_.col + .5f, shift_.row + .5f, .5f, height);
            glDisable(GL_TEXTURE_2D);
        }
    }
    for (const auto house : safehouses_)
    {
        const float hx = house.col + .5f, hz = house.row + .5f;
        if ((hx-x_)*(hx-x_) + (hz-z_)*(hz-z_) > 144.f) continue;
        if (!shelterTextureAttempted_)
        {
            shelterTexture_ = loadSurfaceTexture(executableDirectory() / "assets" / "textures" / "shelter-wood.png");
            shelterTextureAttempted_ = true;
        }
        if (shelterTexture_) { glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, shelterTexture_); }
        if (shelterTexture_) glColor3f(.8f, .78f, .72f); else glColor3f(.21f, .19f, .15f);
        for (float dx : { -1.25f, 1.25f }) for (float dz : { -1.25f, 1.25f })
            part(hx + dx, 0.f, hz + dz, .10f, 1.82f, .10f);
        if (shelterTexture_) glColor3f(.92f, .89f, .82f); else glColor3f(.24f, .22f, .18f);
        for (float side : { -1.f, 1.f }) for (float end : { -1.f, 1.f })
        {
            part(hx + side * 1.45f, 0.f, hz + end * 1.02f, .10f, 1.73f, .86f);
            part(hx + end * 1.02f, 0.f, hz + side * 1.45f, .86f, 1.73f, .10f);
        }
        part(hx, 1.73f, hz, 2.76f, .12f, 2.76f);
        glDisable(GL_TEXTURE_2D);
        glColor3f(.18f, .17f, .14f); stoneCap(hx, hz, 1.42f, 1.85f, 2.24f);
        const GLfloat glow[] = { .55f * lanternFlicker, .30f * lanternFlicker, .10f * lanternFlicker, 1.f };
        const GLfloat dark[] = { 0.f, 0.f, 0.f, 1.f };
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, glow);
        glColor3f(.9f, .63f, .25f); part(hx, 1.47f, hz, .13f, .20f, .13f);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, dark);
    }
    // Place the tablet at the side of the entrance, facing into the passage.
    const bool entranceSouth = open(game.maze, 1, 0);
    glPushMatrix(); glTranslatef(inscriptionX_, 0.f, inscriptionZ_);
    glRotatef(entranceSouth ? 90.f : 0.f, 0.f, 1.f, 0.f);
    glColor3f(.22f, .24f, .22f); part(0.f, 0.f, 0.f, .16f, .53f, .12f);
    glColor3f(.30f, .31f, .28f); part(0.f, .48f, 0.f, .42f, .35f, .08f);
    if ((x_-inscriptionX_)*(x_-inscriptionX_) + (z_-inscriptionZ_)*(z_-inscriptionZ_) < 16.f)
    {
        glDisable(GL_LIGHTING); glColor3f(.66f, .68f, .55f);
        glPushMatrix(); glTranslatef(0.f, .56f, .042f);
        for (int i = 0; i < 3; ++i)
        {
            glPushMatrix(); glTranslatef((i - 1) * .125f, 0.f, 0.f); glScalef(.39f, .39f, .39f);
            shrineGlyph(order_[i], 0); glPopMatrix();
        }
        glPopMatrix(); glEnable(GL_LIGHTING);
    }
    glPopMatrix();
    for (int i = 0; i < 3; ++i)
    {
        const float sx = shrines_[i].col + .5f, sz = shrines_[i].row + .5f;
        // Stones do not block a one-cell-wide passage. Avoid drawing their
        // opaque interior over the camera when the player walks through one.
        if (std::abs(sx - x_) < .22f && std::abs(sz - z_) < .22f) continue;
        glColor3f(.22f, .24f, .21f); box(sx, sz, .21f, .11f);
        glColor3f(.30f, .32f, .28f); box(sx, sz, .16f, .78f);
        stoneCap(sx, sz, .16f, .78f, .92f);
        const float dx = sx - x_, dz = sz - z_;
        if (dx * dx + dz * dz < 36.f)
        {
            const bool activated = (activeStones_ & (1u << i)) != 0;
            const float brightness = activated ? .85f : .55f;
            glDisable(GL_LIGHTING);
            glColor3f(.18f + colors[i][0] * brightness, .18f + colors[i][1] * brightness,
                .18f + colors[i][2] * brightness);
            // Four inscribed faces remain legible from any connected passage.
            for (int face = 0; face < 4; ++face)
            {
                glPushMatrix(); glTranslatef(sx, .47f, sz);
                glRotatef(face * 90.f, 0.f, 1.f, 0.f); glTranslatef(0.f, 0.f, .162f);
                shrineGlyph(i, clues_[i]); glPopMatrix();
            }
            glEnable(GL_LIGHTING);
        }
    }
    if (flash > .05f)
    {
        // The direction and branching remain stable during a strike, but change
        // with each seeded storm event. Depth testing hides it behind the hedge.
        std::mt19937 lightning(game.mazeSeed.value_or(0) ^ storm.event * 0x9e3779b9u);
        const float bearing = static_cast<float>(lightning() % 65536u) / 65536.f * 2.f * pi;
        const float bx = x_ + 15.f * std::sin(bearing), bz = z_ + 15.f * std::cos(bearing);
        const float tangentX = std::cos(bearing), tangentZ = -std::sin(bearing);
        std::array<float, 8> offsets{};
        for (std::size_t i = 1; i < offsets.size(); ++i)
            offsets[i] = offsets[i-1] + (static_cast<int>(lightning() % 201u) - 100) * .004f;
        glDisable(GL_LIGHTING); glDisable(GL_FOG); glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); glDepthMask(GL_FALSE);
        glColor4f(.72f, .81f, 1.f, flash * .85f); glLineWidth(2.f); glBegin(GL_LINE_STRIP);
        for (std::size_t i = 0; i < offsets.size(); ++i)
            glVertex3f(bx + offsets[i] * tangentX, 9.5f - static_cast<float>(i) * .85f,
                bz + offsets[i] * tangentZ);
        glEnd(); glLineWidth(1.f); glBegin(GL_LINE_STRIP);
        glVertex3f(bx + offsets[3] * tangentX, 6.95f, bz + offsets[3] * tangentZ);
        glVertex3f(bx + (offsets[3] + .48f) * tangentX, 6.15f, bz + (offsets[3] + .48f) * tangentZ);
        glVertex3f(bx + (offsets[3] + .70f) * tangentX, 5.25f, bz + (offsets[3] + .70f) * tangentZ);
        glEnd(); glDepthMask(GL_TRUE); glDisable(GL_BLEND); glEnable(GL_FOG); glEnable(GL_LIGHTING);
    }
    const float ex = size.cols - .5f, ez = size.rows - .5f;
    glPushMatrix(); glTranslatef(ex, 0.f, ez);
    if (open(game.maze, size.rows - 1, size.cols - 2)) glRotatef(90.f, 0.f, 1.f, 0.f);
    glColor3f(.38f, .34f, .22f);
    part(-.39f, 0.f, 0.f, .12f, 1.5f, .12f);
    part(.39f, 0.f, 0.f, .12f, 1.5f, .12f);
    part(0.f, 1.42f, 0.f, .9f, .10f, .12f);
    if (!gateOpen_)
    {
        glColor3f(.22f, .20f, .17f);
        for (int bar = -2; bar <= 2; ++bar) part(bar * .14f, 0.f, 0.f, .035f, 1.4f, .04f);
        part(0.f, .60f, 0.f, .75f, .06f, .05f);
    }
    glPopMatrix();
    const float blend = std::clamp(monsterClock_ / monsterStepDuration_, 0.f, 1.f);
    const float mx = monsterFrom_.col + .5f + (monster_.col - monsterFrom_.col) * blend;
    const float mz = monsterFrom_.row + .5f + (monster_.row - monsterFrom_.row) * blend;
    if ((mx-x_)*(mx-x_) + (mz-z_)*(mz-z_) < 144.f)
    {
        if (creatureMesh_ && creatureTexture_)
        {
            if (creatureShader_)
            {
                creatureShader_->setUniform("darkness", darkness_);
                creatureShader_->setUniform("flicker", flicker);
                creatureShader_->setUniform("flash", flash);
                creatureShader_->setUniform("textured", true);
                creatureShader_->setUniform("wetness", .08f);
                creatureShader_->setUniform("gait", creatureGait_);
                creatureShader_->setUniform("time", ambientTime_);
                creatureShader_->setUniform("stride", same(monster_, monsterFrom_) || elapsed_ < 12.f ? 0.f : 1.f);
                creatureShader_->setUniform("chasing", hunter_.mode == HunterMode::Chase ? 1.f : 0.f);
                sf::Shader::bind(creatureShader_.get());
            }
            glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, creatureTexture_);
            glColor3f(.85f, .85f, .85f);
            glPushMatrix(); glTranslatef(mx, 0.f, mz);
            glRotatef(monsterHeading_ * 180.f / pi, 0.f, 1.f, 0.f);
            glCallList(creatureMesh_); glPopMatrix();
            glDisable(GL_TEXTURE_2D);
            if (creatureShader_) sf::Shader::bind(nullptr);
        }
        else
        {
            if (!monsterTextureAttempted_)
            {
                monsterTexture_ = loadSurfaceTexture(executableDirectory() / "assets" / "textures" / "creature.png");
                monsterTextureAttempted_ = true;
            }
            if (monsterTexture_)
            {
                const float dx = mx-x_, dz = mz-z_;
                const float distance = std::max(.01f, std::sqrt(dx*dx + dz*dz));
                const float alignment = std::max(0.f, (dx*std::sin(angle_) + dz*std::cos(angle_)) / distance);
                const float illumination = std::min(1.f, .38f + .5f * std::pow(alignment, 8.f) / (1.f + distance*.12f) + flash*.25f);
                glDisable(GL_LIGHTING); glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, monsterTexture_);
                glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE);
                glColor4f(illumination, illumination, illumination, 1.f);
                glPushMatrix(); glTranslatef(mx, 0.f, mz);
                glRotatef(std::atan2(x_ - mx, z_ - mz) * 180.f / pi, 0.f, 1.f, 0.f);
                glBegin(GL_QUADS);
                glTexCoord2f(0.f, 1.f); glVertex3f(-.62f, 0.f, 0.f);
                glTexCoord2f(1.f, 1.f); glVertex3f(.62f, 0.f, 0.f);
                glTexCoord2f(1.f, 0.f); glVertex3f(.62f, 1.65f, 0.f);
                glTexCoord2f(0.f, 0.f); glVertex3f(-.62f, 1.65f, 0.f);
                glEnd(); glPopMatrix();
                glDepthMask(GL_TRUE); glDisable(GL_BLEND); glDisable(GL_TEXTURE_2D); glEnable(GL_LIGHTING);
            }
            else
            {
                glPushMatrix(); glTranslatef(mx, 0.f, mz);
                glRotatef(std::atan2(x_ - mx, z_ - mz) * 180.f / pi, 0.f, 1.f, 0.f);
                glColor3f(.12f, .10f, .085f);
                const float sway = elapsed_ >= 12.f ? std::sin(elapsed_ * 7.f) * .08f : 0.f;
                part(-.10f, 0.f, sway, .11f, .62f, .13f);
                part(.10f, 0.f, -sway, .11f, .62f, .13f);
                part(0.f, .54f, 0.f, .34f, .55f, .20f);
                part(-.23f, .38f, -sway, .09f, .68f, .10f);
                part(.23f, .38f, sway, .09f, .68f, .10f);
                part(0.f, 1.08f, .01f, .24f, .27f, .20f);
                const GLfloat eyes[] = { .8f, .1f, .035f, 1.f }; glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, eyes);
                glColor3f(.9f, .2f, .08f);
                part(-.055f, 1.23f, .12f, .035f, .035f, .025f);
                part(.055f, 1.23f, .12f, .035f, .035f, .025f);
                const GLfloat dark[] = { 0,0,0,1 }; glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, dark);
                glPopMatrix();
            }
        }
    }
    for (int slot = 0; slot < maxCoopPlayers; ++slot)
    {
        if (!remote_[slot].present) continue;
        const auto& visual = remoteVisual_[slot];
        const float dx = visual.x - x_, dz = visual.z - z_;
        const float distance = dx * dx + dz * dz;
        if (distance > .30f * .30f && distance < 144.f)
        {
            glPushMatrix(); glTranslatef(visual.x, 0.f, visual.z);
            glRotatef(visual.angle * 180.f / pi, 0.f, 1.f, 0.f);
            if (survivorMesh_ && survivorTexture_ && survivorFrames_ > 1 && survivorDuration_ > 0.f)
            {
                const float speed = visual.running ? 1.25f : .72f;
                const float time = visual.moving ? std::fmod(ambientTime_ * speed + slot * .31f, survivorDuration_) : 0.f;
                const auto movingFrames = survivorFrames_ - 1;
                const auto frame = visual.moving ? std::min(movingFrames - 1,
                    static_cast<unsigned int>(time / survivorDuration_ * movingFrames)) : movingFrames;
                const GLfloat tint[4][3] = { { .86f,.90f,.83f }, { .94f,.84f,.77f }, { .81f,.87f,.96f }, { .94f,.90f,.77f } };
                glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, survivorTexture_);
                glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, .4f); glColor3fv(tint[slot]);
                glCallList(survivorMesh_ + frame);
                glDisable(GL_ALPHA_TEST); glDisable(GL_TEXTURE_2D); glPopMatrix();
                continue;
            }
            const float stride = visual.moving ? .08f * std::sin(ambientTime_ * (visual.running ? 10.f : 7.f)) : 0.f;
            glColor3f(.18f, .20f, .18f);
            ellipsoid(-.09f, .26f, stride, .085f, .26f, .085f);
            ellipsoid(.09f, .26f, -stride, .085f, .26f, .085f);
            glColor3f(.13f, .14f, .13f);
            ellipsoid(-.09f, .06f, stride + .03f, .085f, .06f, .13f);
            ellipsoid(.09f, .06f, -stride + .03f, .085f, .06f, .13f);
            const GLfloat coats[4][3] = { { .27f,.30f,.26f }, { .30f,.25f,.21f }, { .23f,.27f,.32f }, { .32f,.29f,.21f } };
            glColor3fv(coats[slot]); ellipsoid(0.f, .72f, 0.f, .21f, .29f, .14f);
            ellipsoid(-.23f, .68f, -stride, .075f, .23f, .075f);
            ellipsoid(.23f, .68f, stride, .075f, .23f, .075f);
            glColor3f(.21f, .23f, .20f); ellipsoid(0.f, 1.06f, 0.f, .135f, .16f, .12f);
            glColor3f(.48f, .39f, .31f); ellipsoid(0.f, 1.06f, .10f, .085f, .105f, .035f);
            const GLfloat glow[] = { .34f, .20f, .07f, 1.f }, dark[] = { 0, 0, 0, 1 };
            glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, glow);
            glColor3f(.9f, .65f, .25f); part(.25f, .35f, .04f, .08f, .12f, .08f);
            glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, dark); glPopMatrix();
        }
    }
    target.popGLStates();
    // Raw 3D rendering changes state behind SFML's cache; re-establish its 2D state.
    target.resetGLStates();

    heading_.setPosition({ 24.f, 16.f }); target.draw(heading_);
    staminaLabel_.setPosition({ static_cast<float>(viewport_.x) - 134.f, 18.f }); target.draw(staminaLabel_);
    sf::RectangleShape breathTrack({ 110.f, 4.f });
    breathTrack.setPosition({ static_cast<float>(viewport_.x) - 134.f, 42.f });
    breathTrack.setFillColor({ 65, 72, 62, 210 }); target.draw(breathTrack);
    sf::RectangleShape breathFill({ 110.f * stamina_, 4.f });
    breathFill.setPosition(breathTrack.getPosition());
    breathFill.setFillColor(exhausted_ ? sf::Color{ 170, 100, 78 } : sf::Color{ 190, 199, 166 });
    target.draw(breathFill);
    soundLabel_.setString(soundMuted_ ? "SOUND OFF" : soundStatus_ < 0 ? "NO AUDIO" : soundStatus_ > 0 ? "SOUND ON" : "SOUND ...");
    soundLabel_.setPosition({ static_cast<float>(viewport_.x) - 134.f, 49.f }); target.draw(soundLabel_);
    seedText_.setString("SEED  " + std::to_string(game.mazeSeed.value_or(0))
        + (networkStatusText_.empty() ? "" : "     /     " + networkStatusText_));
    seedText_.setPosition({ 25.f, 47.f }); target.draw(seedText_);
    if (gateOpen_) objective_.setString("THE GATE IS OPEN   /   STEP THROUGH");
    else if (solved_ == 3) objective_.setString("FIND THE GATE   /   READ ITS INSCRIPTION");
    else if (!inscriptionRead_) objective_.setString("EXAMINE THE ENTRANCE INSCRIPTION   /   E");
    else if (puzzle_.stoneKind == StonePuzzleKind::Balance)
        objective_.setString("LIGHT STONES TO TOTAL " + std::to_string(puzzle_.balanceTarget) + "   /   E TO TOGGLE");
    else objective_.setString("TOUCH THE STONES IN ORDER   /   " + std::to_string(solved_) + "/3");
    objective_.setPosition({ 24.f, static_cast<float>(viewport_.y) - 103.f }); target.draw(objective_);
    if (inscriptionRead_)
    {
        std::string journal = puzzle_.stoneKind == StonePuzzleKind::Balance ? "GATE ORDER  " : "SEQUENCE  ";
        for (int i = 0; i < 3; ++i)
        {
            if (i) journal += "   /   ";
            const int stone = order_[i];
            journal += names[stone];
            journal += " ";
            journal += discoveredClues_[stone] ? std::to_string(clues_[stone]) : "?";
            if (activeStones_ & (1u << stone)) journal += "*";
        }
        journal_.setString(journal);
        journal_.setPosition({ 24.f, static_cast<float>(viewport_.y) - 76.f }); target.draw(journal_);
    }
    hint_.setString("WASD move  |  Mouse look  |  Shift run  |  E touch  |  M sound  |  B motion");
    hint_.setPosition({ 24.f, static_cast<float>(viewport_.y) - 45.f }); target.draw(hint_);
    const int nearby = nearbyShrine();
    if (nearGate(game) && solved_ == 3 && !gateOpen_)
    {
        feedback_.setString("GATE CODE  " + gateInput_ + std::string("___").substr(gateInput_.size()) + "  /  1-9, Enter, Backspace");
        feedback_.setPosition({ 24.f, 78.f }); target.draw(feedback_);
        const char* rule = gateRule_ == GateRule::Forward ? "INSCRIPTION: Read the stone numbers in order."
            : gateRule_ == GateRule::Reverse ? "INSCRIPTION: Read the stone numbers in reverse order."
            : gateRule_ == GateRule::Advance ? "INSCRIPTION: Add 1 to each number; 9 becomes 1."
            : gateRule_ == GateRule::Rotate ? "INSCRIPTION: Read second, then third, then first."
            : "INSCRIPTION: Subtract each number from 10, in order.";
        feedback_.setString(rule); feedback_.setPosition({ 24.f, 106.f }); target.draw(feedback_);
        if (messageClock_ > 0.f)
        {
            feedback_.setString(notice_); feedback_.setPosition({ 24.f, 134.f }); target.draw(feedback_);
        }
    }
    else if (nearby >= 0)
    {
        const char* action = puzzle_.stoneKind == StonePuzzleKind::Balance && solved_ != 3
            ? ((activeStones_ & (1u << nearby)) ? "E to extinguish" : "E to light") : "E to touch";
        feedback_.setString(std::string(names[nearby]) + " STONE  /  engraved " + std::to_string(clues_[nearby]) + "  /  " + action);
        feedback_.setPosition({ 24.f, 78.f }); target.draw(feedback_);
        if (messageClock_ > 0.f)
        {
            feedback_.setString(notice_); feedback_.setPosition({ 24.f, 106.f }); target.draw(feedback_);
        }
    }
    else if (nearInscription() && !inscriptionRead_)
    {
        feedback_.setString("ENTRANCE INSCRIPTION  /  E to read");
        feedback_.setPosition({ 24.f, 78.f }); target.draw(feedback_);
        if (messageClock_ > 0.f)
        {
            feedback_.setString(notice_); feedback_.setPosition({ 24.f, 106.f }); target.draw(feedback_);
        }
    }
    else if (safehouseAt(game.player, safehouses_) >= 0)
    {
        feedback_.setString("SAFEHOUSE  /  THE CREATURE CANNOT ENTER");
        feedback_.setPosition({ 24.f, 78.f }); target.draw(feedback_);
        if (messageClock_ > 0.f)
        {
            feedback_.setString(notice_); feedback_.setPosition({ 24.f, 106.f }); target.draw(feedback_);
        }
    }
    else if (messageClock_ > 0.f)
    {
        feedback_.setString(notice_); feedback_.setPosition({ 24.f, 78.f }); target.draw(feedback_);
    }
    sf::RectangleShape crosshair({ 4.f, 4.f }); crosshair.setFillColor({ 230, 225, 196, 180 });
    crosshair.setPosition({ viewport_.x * .5f - 2.f, viewport_.y * .5f - 2.f }); target.draw(crosshair);
}
