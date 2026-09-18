#pragma once

#include <string>
#include <vector>

#include "core/Camera.h"
#include "core/Input.h"
#include "core/SelfTest.h"
#include "rendering/EntityRenderer.h"
#include "rendering/TextureManager.h"
#include "rendering/TileRenderer.h"
#include "simulation/Economy.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationClock.h"
#include "ui/UI.h"
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
    void drawHighlight();
    void drawPathTest();
    void drawCommutePath();
    void drawPollutionOverlay();
    void drawLandValueOverlay();
    void drawHousingOverlay();
    void drawUtilitiesOverlay();
    void drawBusRoutes();

    World world;
    urbania::Camera camera;
    urbania::Input input;
    SimulationClock simulationClock;
    Simulation simulation;
    urbania::SelfTest selfTest;

    urbania::TextureManager textureManager;
    urbania::TileRenderer tileRenderer;
    urbania::EntityRenderer entityRenderer;
    urbania::UI ui;

    TileType selectedBuildType = TileType::Road;
    bool demolishMode = false;
    bool busStopMode = false;
    bool routeMode = false;
    bool pollutionOverlay = false;
    bool landValueOverlay = false;
    bool housingOverlay = false;
    bool utilitiesOverlay = false;
    bool showDashboard = true;
    bool showSelfTestModal = false;

    std::vector<int> currentRouteStops;
    std::string transitMessage;
    float transitMessageTimer = 0.0f;

    // Temporary A* debug test: path between the first and last road
    // tiles, recomputed only when the world changes.
    std::vector<urbania::TileCoordinate> pathTest;
    bool pathTestDirty = true;
};
