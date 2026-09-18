#pragma once

#include <vector>

#include "core/Camera.h"
#include "core/Input.h"
#include "core/SelfTest.h"
#include "simulation/Economy.h"
#include "simulation/Simulation.h"
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
    void recomputePathTest();
    void drawWorld();
    void drawHighlight();
    void drawPathTest();
    void drawCommutePath();
    void drawCitizens();
    void drawVehicles();
    void drawPollutionOverlay();
    void drawLandValueOverlay();
    void drawDebugText();
    void drawSelfTest();

    World world;
    urbania::Camera camera;
    urbania::Input input;
    SimulationClock simulationClock;
    Simulation simulation;
    urbania::SelfTest selfTest;

    TileType selectedBuildType = TileType::Road;
    bool demolishMode = false;
    bool pollutionOverlay = false;
    bool landValueOverlay = false;

    // Temporary A* debug test: path between the first and last road
    // tiles, recomputed only when the world changes.
    std::vector<urbania::TileCoordinate> pathTest;
    bool pathTestDirty = true;
};
