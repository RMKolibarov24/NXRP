#include <SFML/Graphics.hpp>
#include <array>
#include <atomic>
#include <chrono>
#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <cassert>
#include <cmath>
#include <iostream>
#define private public
#include "MazeApplication.h"
#undef private
#include "Maze.Game.h"
#include "GameView.cpp"
#include "MazeApplication.cpp"

#ifdef NDEBUG
#error The co-op smoke test requires assertions to be enabled.
#endif

int main()
{
    // Stair rise and gallery transition must agree with collision/camera height.
    const std::vector<MazePosition> refugeProbe{ { 4, 4 } };
    float stairHeight = 0.f;
    for (int step = 0; step <= 100; ++step)
    {
        const float z = 4.5f + refugeScale * (.30f + .85f * step / 100.f);
        const float next = refugeFloorHeight(4.5f + .98f * refugeScale, z, stairHeight, refugeProbe);
        assert(next >= stairHeight - .0001f && next - stairHeight < .03f);
        stairHeight = next;
    }
    assert(std::abs(stairHeight - refugeUpperFloor) < .001f);
    assert(refugeFloorHeight(4.5f, 4.5f + 1.15f * refugeScale, stairHeight, refugeProbe) == refugeUpperFloor);
    for (int step = 100; step >= 0; --step)
        stairHeight = refugeFloorHeight(4.5f + .98f * refugeScale, 4.5f + refugeScale * (.30f + .85f * step / 100.f), stairHeight, refugeProbe);
    assert(stairHeight < .001f);
    sf::Font font; assert(font.openFromFile("assets/fonts/SourceSans3-Semibold.ttf"));
    sf::RenderWindow window(sf::VideoMode({640, 560}), "Co-op smoke test"); window.setVisible(false);
    Menu menu(font); menu.resize({640,560});
    sf::Event::MouseButtonPressed click{}; click.button = sf::Mouse::Button::Left; click.position = {320,405};
    assert(menu.handleEvent(sf::Event{click}, window) == MenuAction::Multiplayer);
    menu.showMultiplayer(); click.position = {200,344};
    assert(menu.handleEvent(sf::Event{click}, window) == MenuAction::HostGame);
    click.position = {430,344}; assert(menu.handleEvent(sf::Event{click}, window) == MenuAction::JoinGame);
    sf::Event::KeyPressed clear{}; clear.code = sf::Keyboard::Key::Delete;
    menu.handleEvent(sf::Event{clear}, window);
    for (char character : std::string("tcp://example.com:1234")) {
        sf::Event::TextEntered text{}; text.unicode = static_cast<char32_t>(character);
        menu.handleEvent(sf::Event{text}, window);
    }
    assert(menu.multiplayerAddress() == "tcp://example.com:1234");

    constexpr std::uint16_t testPort = 24817; // Do not interrupt a game hosted on 24816.
    MazeApplication host(font, ""), first(font, ""), second(font, ""), third(font, ""), extra(font, "");
    std::array<MazeApplication*, 4> guests{ &first, &second, &third, &extra };
    host.game_.selectedSeed = 42; host.beginNewGame();
    host.networkRole_ = MazeApplication::NetworkRole::Host; host.networkGeneration_ = 1;
    host.gameView_.update(host.game_, .01f, GameInput{}); host.network_.host(testPort);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    auto pump = [&](int steps) {
        for (int i = 0; i < steps; ++i) {
            host.updateNetwork(.02f);
            for (auto* guest : guests) guest->updateNetwork(.02f);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    };
    auto join = [&](MazeApplication& guest) {
        guest.networkRole_ = MazeApplication::NetworkRole::Guest;
        guest.network_.join("127.0.0.1", testPort); pump(100);
    };
    join(first); join(second); join(third); pump(100);
    assert(host.network_.status() == Multiplayer::Status::Connected);
    assert(first.networkSlot_ == 1 && second.networkSlot_ == 2 && third.networkSlot_ == 3);
    assert(host.network_.connectedMask() == 14u);
    for (int i = 0; i < 3; ++i) {
        auto& guest = *guests[i];
        assert(guest.joinedGeneration_ == 1 && guest.game_.phase == GamePhase::Playing);
        assert(guest.game_.mazeSeed == host.game_.mazeSeed);
        assert(guest.gameView_.playerCount() == 4);
        for (int slot = 0; slot < 4; ++slot)
            assert(guest.gameView_.remote_[slot].present == (slot != guest.networkSlot_));
    }
    assert(host.gameView_.playerCount() == 4);
    join(extra); pump(100);
    assert(extra.networkRole_ == MazeApplication::NetworkRole::Solo);
    assert(extra.menu_.notice_.find("four players") != std::string::npos);

    auto& hv = host.gameView_; auto& gv = first.gameView_;
    const auto beforeMalformed = gv.worldSnapshot();
    assert(!gv.applyWorldSnapshot(first.game_, "not a valid snapshot", first.networkSlot_));
    assert(gv.worldSnapshot() == beforeMalformed);
    const float before = gv.angle_; gv.look(40.f, 0.f); assert(gv.angle_ < before);
    GameInput turn; turn.turn = 1.f; const float beforeKey = gv.angle_;
    gv.update(first.game_, .05f, turn); assert(gv.angle_ < beforeKey);
    gv.angle_ = 0.f; gv.x_ = gv.z_ = 4.5f;
    for (int r=3;r<=5;++r) for(int c=3;c<=5;++c) first.game_.maze.at(r,c)=MazeCell::Passage;
    first.game_.player = {4,4}; GameInput strafe; strafe.strafe = 1.f;
    gv.update(first.game_, .05f, strafe); assert(gv.x_ < 4.5f);
    host.game_.phase = GamePhase::Paused; pump(30);
    for (int i = 0; i < 3; ++i) assert(guests[i]->networkPaused_);
    host.game_.phase = GamePhase::Playing; pump(30);
    for (int i = 0; i < 3; ++i) assert(!guests[i]->networkPaused_);
    host.restartCurrentGame(); hv.update(host.game_, .01f, GameInput{}); pump(60);
    for (int i = 0; i < 3; ++i) {
        assert(guests[i]->joinedGeneration_ == 2);
        assert(guests[i]->gameView_.x_ == .5f && guests[i]->gameView_.z_ == .5f);
    }
    auto sendKey = [&](MazeApplication& guest, char key) {
        guest.sendPlayer(); assert(guest.network_.send(std::string("K ")+key)); pump(40);
    };
    sendKey(first, 'E');
    assert(hv.inscriptionRead_ && first.gameView_.inscriptionRead_ && second.gameView_.inscriptionRead_ && third.gameView_.inscriptionRead_);
    for (int step=0; step<3; ++step) {
        auto& guest = *guests[step]; const int stone = hv.order_[step];
        guest.gameView_.x_ = hv.shrines_[stone].col+.5f; guest.gameView_.z_ = hv.shrines_[stone].row+.5f;
        guest.game_.player = hv.shrines_[stone]; sendKey(guest, 'E');
        assert(hv.solved_ == step+1);
        for (int i=0; i<3; ++i) assert(guests[i]->gameView_.solved_ == step+1 && guests[i]->gameView_.discoveredClues_[stone]);
    }
    const auto size = host.game_.maze.size();
    MazePosition neighbor{size.rows-1,size.cols-2};
    if (host.game_.maze.at(neighbor.row,neighbor.col)==MazeCell::Wall) neighbor={size.rows-2,size.cols-1};
    third.gameView_.x_=neighbor.col+.5f; third.gameView_.z_=neighbor.row+.5f; third.game_.player=neighbor;
    for (int digit : hv.gateCode_) sendKey(third, static_cast<char>('0'+digit));
    sendKey(third, 'N');
    assert(hv.gateOpen_);
    for (int i=0; i<3; ++i) assert(guests[i]->gameView_.gateOpen_);

    // A wall cannot close on any guest, including the third guest.
    hv.updateShift(host.game_,24.f); hv.updateShift(host.game_,2.f);
    assert(hv.shiftPhase_ == GameView::ShiftPhase::Open);
    third.gameView_.x_=hv.shift_.col+.5f; third.gameView_.z_=hv.shift_.row+.5f; third.game_.player=hv.shift_;
    third.sendPlayer(); pump(40); hv.updateShift(host.game_,24.f);
    assert(hv.shiftPhase_ == GameView::ShiftPhase::Open);

    // Visiting a refuge records a personal checkpoint and protects its occupant.
    assert(!hv.safehouses_.empty());
    const auto refuge = hv.safehouses_[0];
    const float houseX = refuge.col+.5f, houseZ = refuge.row+.5f;
    // Safehouse doors start closed, block the threshold and can be opened.
    assert(!hv.walkable(host.game_, houseX, houseZ - 1.45f * refugeScale));
    hv.toggleDoor(0, 2);
    hv.updateDoors(1.f);
    assert(hv.doors_[0][2].open);
    assert(hv.walkable(host.game_, houseX, houseZ - 1.45f * refugeScale));
    assert(!hv.walkable(host.game_, houseX + 1.02f * refugeScale, houseZ - 1.45f * refugeScale));
    second.gameView_.x_=refuge.col+.5f; second.gameView_.z_=refuge.row+.5f; second.game_.player=refuge;
    second.sendPlayer(); pump(40);
    assert(hv.checkpoints_[second.networkSlot_]==0 && second.gameView_.checkpoints_[second.networkSlot_]==0);
    hv.elapsed_=13.f; hv.monster_=hv.monsterFrom_=refuge; hv.monsterClock_=0.f;
    hv.updateMonster(host.game_,.001f,false,false); pump(20);
    assert(second.gameView_.seenRemoteCaptures_==0);

    // Capture guest two outside the refuge without teleporting either other guest.
    const auto victim = hv.shrines_[hv.order_[1]];
    second.gameView_.x_=victim.col+.5f; second.gameView_.z_=victim.row+.5f; second.game_.player=victim;
    second.sendPlayer(); pump(40);
    hv.elapsed_=13.f; hv.monster_=hv.monsterFrom_=victim; hv.monsterClock_=0.f;
    const auto firstCaptures=first.gameView_.seenRemoteCaptures_, thirdCaptures=third.gameView_.seenRemoteCaptures_;
    hv.updateMonster(host.game_,.001f,false,false); pump(40);
    assert(second.gameView_.x_==refuge.col+.5f && second.gameView_.z_==refuge.row+.5f && second.gameView_.seenRemoteCaptures_>0);
    assert(first.gameView_.seenRemoteCaptures_==firstCaptures && third.gameView_.seenRemoteCaptures_==thirdCaptures);

    const int recycledSlot = second.networkSlot_;
    second.endNetwork(); pump(70);
    assert(host.networkRole_==MazeApplication::NetworkRole::Host && host.network_.connectedMask()==10u);
    assert(first.gameView_.playerCount()==3 && third.gameView_.playerCount()==3);
    join(second); pump(80);
    assert(second.networkSlot_==recycledSlot && second.gameView_.seenRemoteCaptures_==0);
    assert(first.gameView_.playerCount()==4 && third.gameView_.playerCount()==4);
    third.gameView_.x_=size.cols-.5f; third.gameView_.z_=size.rows-.5f;
    third.game_.player={size.rows-1,size.cols-1}; third.sendPlayer(); pump(70);
    assert(host.game_.phase==GamePhase::Won);
    for (int i=0; i<3; ++i) assert(guests[i]->game_.phase==GamePhase::Won);

    // A different seed selects the balance variant and shares lit stones.
    host.game_.selectedSeed = 43; host.beginNewGame(); ++host.networkGeneration_;
    hv.update(host.game_,.01f,GameInput{}); pump(70);
    assert(hv.puzzle_.stoneKind == StonePuzzleKind::Balance);
    for (int i=0; i<3; ++i) assert(guests[i]->joinedGeneration_==3);
    sendKey(first, 'E');
    unsigned answer = 0;
    for (unsigned mask=1; mask<8; ++mask)
        if (stoneBalance(hv.puzzle_,mask)==hv.puzzle_.balanceTarget) answer=mask;
    assert(answer != 0);
    int touched = 0;
    for (int stone=0; stone<3; ++stone) if (answer & (1u << stone))
    {
        auto& guest = *guests[touched++];
        guest.gameView_.x_=hv.shrines_[stone].col+.5f; guest.gameView_.z_=hv.shrines_[stone].row+.5f;
        guest.game_.player=hv.shrines_[stone]; sendKey(guest,'E');
        for (int i=0; i<3; ++i) assert(guests[i]->gameView_.activeStones_==hv.activeStones_);
    }
    assert(hv.solved_==3);
    for (int i=0; i<3; ++i) assert(guests[i]->gameView_.solved_==3);
    // Physical games are host-authoritative and shared between actual TCP peers.
    const auto gamesHouse = hv.safehouses_[0];
    for (auto* guest : { &first, &second })
    {
        guest->gameView_.x_ = gamesHouse.col + .5f - .7f;
        guest->gameView_.z_ = gamesHouse.row + .5f + .5f;
        guest->gameView_.elevation_ = refugeUpperFloor;
        guest->gameView_.angle_ = 0.f; guest->game_.player = gamesHouse;
        guest->sendPlayer();
    }
    pump(30); sendKey(first, 'G');
    assert(hv.refugeBalls_[0][1].carrier == first.networkSlot_);
    sendKey(second, 'G');
    assert(hv.refugeBalls_[0][1].carrier == first.networkSlot_);
    for (int i=0; i<3; ++i) assert(guests[i]->gameView_.refugeBalls_[0][1].carrier == first.networkSlot_);
    sendKey(first, 'G');
    assert(hv.refugeBalls_[0][1].carrier == -1 && hv.refugeBalls_[0][1].vy > 0.f);
    auto& basket = hv.refugeBalls_[0][1];
    basket.x = -.55f; basket.z = 1.53f; basket.y = 1.13f;
    basket.vx = basket.vz = 0.f; basket.vy = -1.f;
    hv.updateRefugeGames(.03f); assert(basket.scoreA == 1);
    auto& football = hv.refugeBalls_[0][0];
    football.x = -.55f; football.z = 1.91f; football.vz = 2.f;
    hv.updateRefugeGames(.03f); assert(football.scoreA == 1 && football.scoreB == 0);
    football.x = -.55f; football.z = -1.91f; football.vz = -2.f;
    hv.updateRefugeGames(.03f); assert(football.scoreA == 1 && football.scoreB == 1);
    pump(30);
    for (int i=0;i<3;++i)
    { assert(guests[i]->gameView_.refugeBalls_[0][1].scoreA == 1); assert(guests[i]->gameView_.refugeBalls_[0][0].scoreB == 1); }
    // Ground-floor players cannot take upstairs balls.
    first.gameView_.elevation_ = 0.f; first.sendPlayer(); pump(20); sendKey(first, 'G');
    assert(basket.carrier == -1);
    // Guests pull the physical cabinet handle; cooldown is shared per cabinet.
    first.gameView_.x_ = gamesHouse.col + .5f - .56f * refugeScale;
    first.gameView_.z_ = gamesHouse.row + .5f - .85f * refugeScale + .50f;
    first.gameView_.angle_ = 3.14159265f; first.gameView_.elevation_ = 0.f;
    first.sendPlayer(); pump(20); sendKey(first, 'E');
    assert(hv.refugeSlots_[0].spins == 1);
    for (int i=0;i<3;++i) assert(guests[i]->gameView_.refugeSlots_[0].symbols == hv.refugeSlots_[0].symbols);
    sendKey(first, 'E'); assert(hv.refugeSlots_[0].spins == 1);
    if (hv.refugeSlots_.size() > 1) assert(hv.refugeSlots_[1].spins == 0);
    first.gameView_.elevation_ = refugeUpperFloor;
    hv.elapsed_ += 3.f; first.sendPlayer(); pump(20); sendKey(first, 'E');
    assert(hv.refugeSlots_[0].spins == 1);
    // Walk up the stair, onto the gallery, back to the stair and down again.
    hv.x_ = gamesHouse.col + .5f + .98f * refugeScale;
    hv.z_ = gamesHouse.row + .5f + .20f * refugeScale;
    hv.elevation_ = 0.f; hv.angle_ = 0.f; hv.elapsed_ = 0.f;
    host.game_.player = gamesHouse;
    GameInput stairWalk; stairWalk.forward = 1.f;
    for (int i=0;i<30;++i) hv.update(host.game_,.05f,stairWalk);
    assert(hv.elevation_ > refugeUpperFloor - .01f);
    stairWalk.forward = 0.f; stairWalk.strafe = 1.f;
    for (int i=0;i<16;++i) hv.update(host.game_,.05f,stairWalk);
    assert(hv.elevation_ == refugeUpperFloor && hv.x_ < gamesHouse.col + .5f + .65f * refugeScale);
    stairWalk.strafe = -1.f;
    for (int i=0;i<16;++i) hv.update(host.game_,.05f,stairWalk);
    stairWalk.strafe = 0.f; stairWalk.forward = -1.f;
    for (int i=0;i<30;++i) hv.update(host.game_,.05f,stairWalk);
    assert(hv.elevation_ < .01f);
    // A complete pickup/throw arc can score; scoring isn't just a HUD command.
    hv.x_ = gamesHouse.col + .5f - .55f; hv.z_ = gamesHouse.row + .5f - .713f;
    hv.elevation_ = refugeUpperFloor; hv.angle_ = 0.f;
    basket.x = -.55f; basket.z = -.713f; basket.y = .13f; basket.carrier = -1;
    basket.vx = basket.vy = basket.vz = 0.f;
    hv.playRefugeBall(true, 0); assert(basket.carrier == 0);
    hv.updateRefugeGames(.01f); hv.playRefugeBall(true, 0);
    const unsigned beforeArc = basket.scoreA;
    hv.updateRefugeGames(1.2f); assert(basket.scoreA == beforeArc + 1);
    basket.carrier = first.networkSlot_; hv.remote_[first.networkSlot_].present = false;
    hv.updateRefugeGames(.01f); assert(basket.carrier == -1);
    Multiplayer unavailable; unavailable.join("127.0.0.1", 1);
    for (int i=0; i<400 && unavailable.status()==Multiplayer::Status::Connecting; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    assert(unavailable.status()==Multiplayer::Status::Error);
    // Render current cabin geometry on both floors for visual review.
    host.game_.phase = GamePhase::Playing;
    const auto renderHouse = hv.safehouses_.front();
    hv.x_ = renderHouse.col + .5f; hv.z_ = renderHouse.row + .5f;
    host.game_.player = renderHouse; hv.angle_ = 3.8f; hv.pitch_ = 0.f;
    hv.elevation_ = 0.f; hv.resize({640,560});
    sf::RenderTexture refugeTarget({640,560}, sf::ContextSettings{24});
    hv.draw(refugeTarget, host.game_); refugeTarget.display();
    assert(refugeTarget.getTexture().copyToImage().saveToFile("build/refuge-ground.png"));
    hv.elevation_ = refugeUpperFloor; hv.angle_ = 0.f; hv.pitch_ = -.12f;
    hv.draw(refugeTarget, host.game_); refugeTarget.display();
    assert(refugeTarget.getTexture().copyToImage().saveToFile("build/refuge-upper.png"));
    std::cout << "Screen-space ray tracing active: " << hv.rayTracingActive() << "\n";
    std::cout << "Four-player joins, capacity, both puzzles, refuges, captures, shifting, reconnect, and victory passed.\n";
}
