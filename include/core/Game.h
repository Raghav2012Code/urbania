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
    void drawHousingOverlay();
    void drawBusStops();
    void drawBusRoutes();
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
    bool busStopMode = false;
    bool routeMode = false;
    bool pollutionOverlay = false;
    bool landValueOverlay = false;
    bool housingOverlay = false;

    std::vector<int> currentRouteStops;
    std::string transitMessage;
    float transitMessageTimer = 0.0f;

    // Temporary A* debug test: path between the first and last road
    // tiles, recomputed only when the world changes.
    std::vector<urbania::TileCoordinate> pathTest;
    bool pathTestDirty = true;
};
