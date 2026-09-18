#include "core/Game.h"

#include <algorithm>
#include <string>

#include "raylib.h"
#include "simulation/Pathfinder.h"
#include "world/Tile.h"

namespace {

constexpr Color HIGHLIGHT_BORDER = { 255, 203, 5, 255 };
constexpr Color PREVIEW_FILL = { 255, 255, 255, 110 };
constexpr Color BLOCKED_BORDER = { 220, 50, 50, 255 };
constexpr Color DEMOLISH_FILL = { 220, 50, 50, 110 };
constexpr Color PATH_TEST_OUTLINE = { 30, 100, 255, 255 };
constexpr Color PATH_COMMUTE_DOT = { 150, 50, 200, 255 };
constexpr Color UNAVAILABLE_BORDER = { 150, 150, 150, 255 };

Color tilePreviewColor(TileType type)
{
    switch (type)
    {
    case TileType::Road: return Color{ 130, 135, 145, 255 };
    case TileType::Residential: return Color{ 70, 130, 220, 255 };
    case TileType::Commercial: return Color{ 240, 160, 40, 255 };
    case TileType::Industrial: return Color{ 175, 75, 75, 255 };
    case TileType::Park: return Color{ 35, 140, 60, 255 };
    default: return Color{ 120, 200, 80, 255 };
    }
}

}  // namespace

bool Game::initialize()
{
    const int screenWidth = 1920;
    const int screenHeight = 1080;

    InitWindow(screenWidth, screenHeight, "Urbania");
    SetTargetFPS(60);

    SetWindowPosition((GetMonitorWidth(0) - screenWidth) / 2,
                      (GetMonitorHeight(0) - screenHeight) / 2);
    RestoreWindow();
    SetWindowFocused();

    textureManager.initialize();

    if (!simulation.initialize(world))
    {
        return false;
    }

    return IsWindowReady();
}

void Game::update(float deltaTime)
{
    camera.update(deltaTime);
    input.update(camera);
    handleSimulationInput();

    ui.update(simulationClock, selectedBuildType, demolishMode, busStopMode,
              routeMode, pollutionOverlay, landValueOverlay, housingOverlay,
              utilitiesOverlay, showDashboard, showSelfTestModal);

    simulationClock.update(deltaTime);
    simulation.update(simulationClock.getSimulationDeltaTime());
    handleBuildInput();

    if (transitMessageTimer > 0.0f)
    {
        transitMessageTimer -= deltaTime;
        if (transitMessageTimer <= 0.0f)
        {
            transitMessage.clear();
        }
    }

    if (pathTestDirty)
    {
        recomputePathTest();
        pathTestDirty = false;
    }
}

void Game::draw()
{
    ClearBackground(Color{ 18, 22, 32, 255 });

    // 1. World Space Rendering
    camera.begin();
    tileRenderer.renderWorld(world, textureManager, simulation.getCongestion(),
                             simulation.getTransit());
    drawBusRoutes();
    if (pollutionOverlay)
    {
        drawPollutionOverlay();
    }
    if (landValueOverlay)
    {
        drawLandValueOverlay();
    }
    if (housingOverlay)
    {
        drawHousingOverlay();
    }
    if (utilitiesOverlay)
    {
        drawUtilitiesOverlay();
    }
    drawHighlight();
    drawPathTest();
    drawCommutePath();
    entityRenderer.renderCitizens(simulation.getPopulation().getCitizens(), world.getTileSize());
    entityRenderer.renderVehicles(simulation.getTraffic(), world.getTileSize());
    entityRenderer.renderBuses(simulation.getTransit(), world.getTileSize());
    camera.end();

    // 2. Screen Space UI & HUD
    ui.draw(world, simulation, simulationClock, selectedBuildType, demolishMode,
            busStopMode, routeMode, pollutionOverlay, landValueOverlay,
            housingOverlay, utilitiesOverlay, showDashboard, showSelfTestModal, input.hovered(),
            currentRouteStops, transitMessage, transitMessageTimer, selfTest);
}

void Game::shutdown()
{
    textureManager.shutdown();
    simulation.shutdown();
    CloseWindow();
}

void Game::handleSimulationInput()
{
    if (IsKeyPressed(KEY_SPACE))
    {
        if (simulationClock.isPaused())
        {
            simulationClock.resume();
        }
        else
        {
            simulationClock.pause();
        }
    }
    else if (IsKeyPressed(KEY_F1))
    {
        simulationClock.setTimeScale(SimulationClock::NORMAL_SPEED);
    }
    else if (IsKeyPressed(KEY_F2))
    {
        simulationClock.setTimeScale(SimulationClock::FAST_SPEED);
    }
    else if (IsKeyPressed(KEY_F3))
    {
        simulationClock.setTimeScale(SimulationClock::VERY_FAST_SPEED);
    }
    else if (IsKeyPressed(KEY_F4))
    {
        simulationClock.setTimeScale(SimulationClock::EXTREMELY_FAST_SPEED);
    }
    else if (IsKeyPressed(KEY_F5))
    {
        pollutionOverlay = !pollutionOverlay;
    }
    else if (IsKeyPressed(KEY_F6))
    {
        landValueOverlay = !landValueOverlay;
    }
    else if (IsKeyPressed(KEY_F7))
    {
        housingOverlay = !housingOverlay;
    }
    else if (IsKeyPressed(KEY_F8))
    {
        utilitiesOverlay = !utilitiesOverlay;
    }
    else if (IsKeyPressed(KEY_TAB))
    {
        showDashboard = !showDashboard;
    }
    else if (IsKeyPressed(KEY_F9))
    {
        showSelfTestModal = !showSelfTestModal;
        if (showSelfTestModal)
        {
            selfTest.run(world, simulation.getEconomy(), simulation);
            pathTestDirty = true;
        }
    }
}

void Game::handleBuildInput()
{
    if (IsKeyPressed(KEY_ONE))
    {
        selectedBuildType = TileType::Road;
        demolishMode = false;
        busStopMode = false;
        routeMode = false;
        currentRouteStops.clear();
    }
    else if (IsKeyPressed(KEY_TWO))
    {
        selectedBuildType = TileType::Residential;
        demolishMode = false;
        busStopMode = false;
        routeMode = false;
        currentRouteStops.clear();
    }
    else if (IsKeyPressed(KEY_THREE))
    {
        selectedBuildType = TileType::Commercial;
        demolishMode = false;
        busStopMode = false;
        routeMode = false;
        currentRouteStops.clear();
    }
    else if (IsKeyPressed(KEY_FOUR))
    {
        selectedBuildType = TileType::Industrial;
        demolishMode = false;
        busStopMode = false;
        routeMode = false;
        currentRouteStops.clear();
    }
    else if (IsKeyPressed(KEY_FIVE))
    {
        selectedBuildType = TileType::Park;
        demolishMode = false;
        busStopMode = false;
        routeMode = false;
        currentRouteStops.clear();
    }
    else if (IsKeyPressed(KEY_D))
    {
        demolishMode = !demolishMode;
        if (demolishMode)
        {
            busStopMode = false;
            routeMode = false;
            currentRouteStops.clear();
        }
    }
    else if (IsKeyPressed(KEY_B))
    {
        busStopMode = !busStopMode;
        if (busStopMode)
        {
            demolishMode = false;
            routeMode = false;
            currentRouteStops.clear();
        }
    }
    else if (IsKeyPressed(KEY_R))
    {
        const bool isShift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        if (isShift)
        {
            if (simulation.getTransit().deleteLatestRoute())
            {
                transitMessage = "Deleted latest bus route.";
                transitMessageTimer = 3.0f;
            }
            else
            {
                transitMessage = "No bus routes to delete.";
                transitMessageTimer = 3.0f;
            }
        }
        else
        {
            routeMode = !routeMode;
            if (routeMode)
            {
                demolishMode = false;
                busStopMode = false;
                currentRouteStops.clear();
                transitMessage = "Route Mode: Click bus stops, Enter to save, Esc to cancel.";
                transitMessageTimer = 4.0f;
            }
            else
            {
                currentRouteStops.clear();
            }
        }
    }

    if (routeMode)
    {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))
        {
            if (currentRouteStops.size() < urbania::Transit::MIN_ROUTE_STOPS)
            {
                transitMessage = "Route needs at least 2 stops.";
                transitMessageTimer = 3.0f;
            }
            else if (!simulation.getTransit().createRoute(simulation.getRoadNetwork(),
                                                          currentRouteStops))
            {
                transitMessage = "Route invalid: stops not connected by road network.";
                transitMessageTimer = 3.0f;
            }
            else
            {
                transitMessage = TextFormat("Created Bus Route #%d with %d stops.",
                                            simulation.getTransit().getRoutes().back().id,
                                            static_cast<int>(currentRouteStops.size()));
                transitMessageTimer = 3.0f;
                currentRouteStops.clear();
            }
        }
        else if (IsKeyPressed(KEY_ESCAPE))
        {
            currentRouteStops.clear();
            transitMessage = "Cancelled current route draft.";
            transitMessageTimer = 2.0f;
        }
    }

    if (!input.leftClicked())
    {
        return;
    }

    // Prevent clicking world tiles when mouse is hovering over interactive UI panels
    if (ui.isMouseOverUI())
    {
        return;
    }

    const urbania::TileCoordinate hovered = input.hovered();
    if (!hovered.valid)
    {
        return;
    }

    if (busStopMode)
    {
        const bool isShift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        if (isShift)
        {
            if (simulation.getTransit().removeBusStop(hovered))
            {
                simulation.onWorldModified();
                pathTestDirty = true;
            }
        }
        else
        {
            if (simulation.getTransit().addBusStop(world, hovered, simulation.getEconomy()))
            {
                simulation.onWorldModified();
                pathTestDirty = true;
            }
        }
        return;
    }

    if (routeMode)
    {
        if (simulation.getTransit().hasBusStop(hovered))
        {
            const auto* stop = simulation.getTransit().getBusStop(hovered);
            if (stop != nullptr)
            {
                bool alreadyInRoute = false;
                for (int id : currentRouteStops)
                {
                    if (id == stop->id)
                    {
                        alreadyInRoute = true;
                        break;
                    }
                }

                if (alreadyInRoute)
                {
                    if (!currentRouteStops.empty() && currentRouteStops.front() == stop->id &&
                        currentRouteStops.size() >= 2 && currentRouteStops.back() != stop->id)
                    {
                        currentRouteStops.push_back(stop->id);
                        transitMessage = TextFormat("Added Stop #%d to close route loop.", stop->id);
                        transitMessageTimer = 2.0f;
                    }
                    else
                    {
                        transitMessage = "Stop is already in current route.";
                        transitMessageTimer = 2.0f;
                    }
                }
                else
                {
                    currentRouteStops.push_back(stop->id);
                    transitMessage = TextFormat("Added Stop #%d to route (%d total).", stop->id,
                                                static_cast<int>(currentRouteStops.size()));
                    transitMessageTimer = 2.0f;
                }
            }
        }
        return;
    }

    Tile& tile = world.getTile(hovered.x, hovered.y);

    bool changed = false;
    if (demolishMode)
    {
        changed = simulation.getEconomy().tryDemolish(tile);
    }
    else
    {
        changed = simulation.getEconomy().tryBuild(tile, selectedBuildType);
    }

    if (changed)
    {
        simulation.onWorldModified();
        pathTestDirty = true;
    }
}

void Game::drawHighlight()
{
    const urbania::TileCoordinate hovered = input.hovered();
    if (!hovered.valid || ui.isMouseOverUI())
    {
        return;
    }

    const int tileSize = world.getTileSize();
    const int px = hovered.x * tileSize;
    const int py = hovered.y * tileSize;
    const Rectangle rect = { static_cast<float>(px), static_cast<float>(py),
                             static_cast<float>(tileSize), static_cast<float>(tileSize) };

    const Tile& tile = world.getTile(hovered.x, hovered.y);

    if (demolishMode)
    {
        if (Economy::isDeveloped(tile))
        {
            DrawRectangle(px, py, tileSize, tileSize, DEMOLISH_FILL);
            DrawRectangleLinesEx(rect, 2.0f, BLOCKED_BORDER);
        }
        else
        {
            DrawRectangleLinesEx(rect, 2.0f, UNAVAILABLE_BORDER);
        }
        return;
    }

    if (busStopMode)
    {
        const bool isShift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        if (isShift)
        {
            if (simulation.getTransit().hasBusStop(hovered))
            {
                DrawRectangle(px, py, tileSize, tileSize, DEMOLISH_FILL);
                DrawRectangleLinesEx(rect, 2.0f, BLOCKED_BORDER);
            }
            else
            {
                DrawRectangleLinesEx(rect, 2.0f, UNAVAILABLE_BORDER);
            }
        }
        else
        {
            if (simulation.getTransit().canPlaceBusStop(world, hovered) &&
                simulation.getEconomy().canAfford(urbania::Transit::BUS_STOP_COST))
            {
                Color preview = { 50, 150, 240, 100 };
                DrawRectangle(px, py, tileSize, tileSize, preview);
                DrawRectangleLinesEx(rect, 2.0f, HIGHLIGHT_BORDER);
            }
            else
            {
                DrawRectangleLinesEx(rect, 2.0f, BLOCKED_BORDER);
            }
        }
        return;
    }

    if (routeMode)
    {
        if (simulation.getTransit().hasBusStop(hovered))
        {
            DrawRectangle(px, py, tileSize, tileSize, Color{ 255, 230, 50, 120 });
            DrawRectangleLinesEx(rect, 2.0f, HIGHLIGHT_BORDER);
        }
        else
        {
            DrawRectangleLinesEx(rect, 2.0f, UNAVAILABLE_BORDER);
        }
        return;
    }

    if (tile.type != TileType::Grass)
    {
        DrawRectangleLinesEx(rect, 2.0f, BLOCKED_BORDER);
    }
    else if (!simulation.getEconomy().canAfford(selectedBuildType))
    {
        DrawRectangleLinesEx(rect, 2.0f, UNAVAILABLE_BORDER);
    }
    else
    {
        Color preview = tilePreviewColor(selectedBuildType);
        preview.a = 110;
        DrawRectangle(px, py, tileSize, tileSize, preview);
        DrawRectangle(px, py, tileSize, tileSize, PREVIEW_FILL);
        DrawRectangleLinesEx(rect, 2.0f, HIGHLIGHT_BORDER);
    }
}

void Game::recomputePathTest()
{
    urbania::TileCoordinate first{};
    urbania::TileCoordinate last{};
    bool found = false;

    for (int y = 0; y < world.getHeight(); ++y)
    {
        for (int x = 0; x < world.getWidth(); ++x)
        {
            if (world.getTile(x, y).type == TileType::Road)
            {
                if (!found)
                {
                    first = { x, y, true };
                    found = true;
                }
                last = { x, y, true };
            }
        }
    }

    pathTest.clear();
    if (found)
    {
        pathTest = urbania::Pathfinder::findPath(simulation.getRoadNetwork(), first, last);
    }
}

void Game::drawPathTest()
{
    const int tileSize = world.getTileSize();
    for (const urbania::TileCoordinate& step : pathTest)
    {
        DrawRectangleLinesEx({ static_cast<float>(step.x * tileSize),
                               static_cast<float>(step.y * tileSize),
                               static_cast<float>(tileSize), static_cast<float>(tileSize) },
                             2.0f, PATH_TEST_OUTLINE);
    }
}

void Game::drawCommutePath()
{
    const int tileSize = world.getTileSize();
    for (const urbania::TileCoordinate& step : simulation.getCommuteSystem().getSampleRoute())
    {
        DrawCircle(step.x * tileSize + tileSize / 2, step.y * tileSize + tileSize / 2, 4.0f,
                   PATH_COMMUTE_DOT);
    }
}

void Game::drawPollutionOverlay()
{
    const int tileSize = world.getTileSize();
    for (const auto& entry : simulation.getPollution().getPollutionGrid())
    {
        const urbania::TileCoordinate& coord = entry.first;
        const float pollution = entry.second;
        if (pollution <= 0.001f)
        {
            continue;
        }

        const unsigned char alpha = static_cast<unsigned char>(
            std::clamp(pollution * 1.8f, 10.0f, 180.0f));
        const Color smogColor = { 110, 75, 45, alpha };
        DrawRectangle(coord.x * tileSize, coord.y * tileSize, tileSize, tileSize, smogColor);
    }
}

void Game::drawLandValueOverlay()
{
    const int tileSize = world.getTileSize();
    for (const auto& entry : simulation.getLandValue().getLandValueGrid())
    {
        const urbania::TileCoordinate& coord = entry.first;
        const float val = entry.second;

        Color tint;
        if (val >= 50.0f)
        {
            const unsigned char alpha = static_cast<unsigned char>(
                std::clamp((val - 50.0f) * 3.0f, 10.0f, 160.0f));
            tint = { 50, 220, 80, alpha };
        }
        else
        {
            const unsigned char alpha = static_cast<unsigned char>(
                std::clamp((50.0f - val) * 3.0f, 10.0f, 160.0f));
            tint = { 200, 60, 50, alpha };
        }
        DrawRectangle(coord.x * tileSize, coord.y * tileSize, tileSize, tileSize, tint);
    }
}

void Game::drawHousingOverlay()
{
    const int tileSize = world.getTileSize();
    for (const auto& entry : simulation.getHousing().getResidentialGrid())
    {
        const urbania::TileCoordinate& coord = entry.first;
        const int residents = simulation.getPopulation().getResidentsAt(coord.x, coord.y);
        const int capacity = urbania::Housing::CAPACITY_PER_TILE;
        const float occ = (capacity > 0)
                              ? (static_cast<float>(residents) / static_cast<float>(capacity))
                              : 0.0f;

        Color tint;
        if (occ >= 0.8f)
        {
            tint = { 30, 180, 60, 160 };
        }
        else if (occ >= 0.4f)
        {
            tint = { 220, 190, 40, 150 };
        }
        else
        {
            tint = { 80, 170, 230, 130 };
        }
        DrawRectangle(coord.x * tileSize, coord.y * tileSize, tileSize, tileSize, tint);
    }
}

void Game::drawUtilitiesOverlay()
{
    const int tileSize = world.getTileSize();
    for (const auto& entry : simulation.getUtilities().getStatusGrid())
    {
        const urbania::TileCoordinate& coord = entry.first;
        const auto& status = entry.second;

        Color tint;
        if (status.isFullySupplied)
        {
            tint = { 0, 200, 240, 140 };
        }
        else
        {
            tint = { 231, 76, 60, 160 };
        }

        DrawRectangle(coord.x * tileSize, coord.y * tileSize, tileSize, tileSize, tint);
        DrawRectangleLinesEx(Rectangle{ static_cast<float>(coord.x * tileSize),
                                        static_cast<float>(coord.y * tileSize),
                                        static_cast<float>(tileSize),
                                        static_cast<float>(tileSize) },
                             1.0f,
                             status.isFullySupplied ? Color{ 50, 230, 255, 200 }
                                                    : Color{ 255, 100, 80, 220 });
    }
}

void Game::drawBusRoutes()
{
    const int tileSize = world.getTileSize();
    const float halfTile = static_cast<float>(tileSize) / 2.0f;

    const auto& routes = simulation.getTransit().getRoutes();
    const Color routeColors[] = {
        Color{ 180, 50, 220, 200 },
        Color{ 40, 160, 220, 200 },
        Color{ 230, 120, 30, 200 },
        Color{ 50, 200, 100, 200 },
        Color{ 220, 60, 100, 200 }
    };
    const size_t numColors = sizeof(routeColors) / sizeof(routeColors[0]);

    for (size_t r = 0; r < routes.size(); ++r)
    {
        const auto& route = routes[r];
        const Color col = routeColors[r % numColors];
        for (size_t i = 0; i + 1 < route.stopIds.size(); ++i)
        {
            const auto* a = simulation.getTransit().getBusStopById(route.stopIds[i]);
            const auto* b = simulation.getTransit().getBusStopById(route.stopIds[i + 1]);
            if (a != nullptr && b != nullptr && a->tile.valid && b->tile.valid)
            {
                Vector2 start = { a->tile.x * tileSize + halfTile, a->tile.y * tileSize + halfTile };
                Vector2 end = { b->tile.x * tileSize + halfTile, b->tile.y * tileSize + halfTile };
                DrawLineEx(start, end, 3.0f, col);
            }
        }
    }

    if (routeMode && !currentRouteStops.empty())
    {
        for (size_t i = 0; i + 1 < currentRouteStops.size(); ++i)
        {
            const auto* a = simulation.getTransit().getBusStopById(currentRouteStops[i]);
            const auto* b = simulation.getTransit().getBusStopById(currentRouteStops[i + 1]);
            if (a != nullptr && b != nullptr && a->tile.valid && b->tile.valid)
            {
                Vector2 start = { a->tile.x * tileSize + halfTile, a->tile.y * tileSize + halfTile };
                Vector2 end = { b->tile.x * tileSize + halfTile, b->tile.y * tileSize + halfTile };
                DrawLineEx(start, end, 4.0f, Color{ 255, 230, 40, 230 });
            }
        }

        for (size_t i = 0; i < currentRouteStops.size(); ++i)
        {
            const auto* stop = simulation.getTransit().getBusStopById(currentRouteStops[i]);
            if (stop != nullptr && stop->tile.valid)
            {
                DrawCircle(static_cast<int>(stop->tile.x * tileSize + halfTile),
                           static_cast<int>(stop->tile.y * tileSize + halfTile),
                           7.0f, Color{ 255, 220, 0, 200 });
                DrawCircleLines(static_cast<int>(stop->tile.x * tileSize + halfTile),
                                static_cast<int>(stop->tile.y * tileSize + halfTile),
                                7.0f, DARKBLUE);
            }
        }
    }
}
