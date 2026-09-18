#pragma once

#include "core/Camera.h"
#include "core/Input.h"
#include "simulation/Economy.h"
#include "simulation/SimulationClock.h"
#include "world/Tile.h"
#include "world/World.h"

class Game {
public:
    bool initialize();
    void update(float deltaTime);
    void draw();
    void shutdown();

private:
    void handleBuildInput();
    void handleSimulationInput();
    void drawWorld();
    void drawHighlight();
    void drawDebugText();

    World world;
    urbania::Camera camera;
    urbania::Input input;
    Economy economy;
    SimulationClock simulationClock;

    TileType selectedBuildType = TileType::Road;
    bool demolishMode = false;
};
