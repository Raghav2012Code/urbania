#include "core/Game.h"

#include <algorithm>
#include <string>

#include "raylib.h"
#include "simulation/Pathfinder.h"
#include "world/Tile.h"

namespace {

constexpr Color GRASS_FILL = { 126, 217, 87, 255 };
constexpr Color ROAD_FILL = { 105, 105, 105, 255 };
constexpr Color RESIDENTIAL_FILL = { 80, 150, 255, 255 };
constexpr Color COMMERCIAL_FILL = { 255, 170, 30, 255 };
constexpr Color INDUSTRIAL_FILL = { 190, 70, 70, 255 };
constexpr Color PARK_FILL = { 35, 140, 60, 255 };
constexpr Color GRID_LINE = { 0, 0, 0, 30 };
constexpr Color HIGHLIGHT_BORDER = { 255, 203, 5, 255 };
constexpr Color PREVIEW_FILL = { 255, 255, 255, 110 };
constexpr Color BLOCKED_BORDER = { 220, 50, 50, 255 };
constexpr Color DEMOLISH_FILL = { 220, 50, 50, 110 };
constexpr Color PATH_TEST_OUTLINE = { 30, 100, 255, 255 };
constexpr Color PATH_COMMUTE_DOT = { 150, 50, 200, 255 };
constexpr Color CITIZEN_DOT = { 20, 60, 160, 255 };
constexpr Color VEHICLE_FILL = { 180, 30, 30, 255 };
constexpr Color PATH_VEHICLE_OUTLINE = { 255, 140, 0, 255 };
constexpr Color UNAVAILABLE_BORDER = { 150, 150, 150, 255 };

Color tileColor(TileType type)
{
    switch (type)
    {
        case TileType::Grass:
            return GRASS_FILL;
        case TileType::Road:
            return ROAD_FILL;
        case TileType::Residential:
            return RESIDENTIAL_FILL;
        case TileType::Commercial:
            return COMMERCIAL_FILL;
        case TileType::Industrial:
            return INDUSTRIAL_FILL;
        case TileType::Park:
            return PARK_FILL;
    }

    return GRASS_FILL;
}

// Temporary congestion visual: blend road grey toward red as load
// passes capacity. Only the display clamps (0 to 2x); the internal
// congestion value stays unclamped so overload remains measurable.
Color congestedRoadColor(float congestion)
{
    constexpr Color JAM = { 200, 40, 40, 255 };
    const float t = std::clamp(congestion / 2.0f, 0.0f, 1.0f);
    return { static_cast<unsigned char>(ROAD_FILL.r + (JAM.r - ROAD_FILL.r) * t),
             static_cast<unsigned char>(ROAD_FILL.g + (JAM.g - ROAD_FILL.g) * t),
             static_cast<unsigned char>(ROAD_FILL.b + (JAM.b - ROAD_FILL.b) * t), 255 };
}

const char* tileTypeName(TileType type)
{
    switch (type)
    {
        case TileType::Grass:
            return "Grass";
        case TileType::Road:
            return "Road";
        case TileType::Residential:
            return "Residential";
        case TileType::Commercial:
            return "Commercial";
        case TileType::Industrial:
            return "Industrial";
        case TileType::Park:
            return "Park";
    }

    return "Unknown";
}

// Format money as "Rs. 100,000". The "Rs." prefix is used instead of the
// rupee sign because raylib's default font cannot render U+20B9.
std::string formatMoney(int amount)
{
    const std::string digits = std::to_string(amount);
    std::string grouped;
    grouped.reserve(digits.size() + 2);

    int count = 0;
    for (auto it = digits.rbegin(); it != digits.rend(); ++it)
    {
        if (count > 0 && count % 3 == 0)
        {
            grouped.push_back(',');
        }
        grouped.push_back(*it);
        ++count;
    }

    return "Rs. " + std::string(grouped.rbegin(), grouped.rend());
}

}  // namespace

bool Game::initialize()
{
    const int screenWidth = 1920;
    const int screenHeight = 1080;

    InitWindow(screenWidth, screenHeight, "Urbania");
    SetTargetFPS(60);

    // Guarantee the window opens visibly: center it on the primary
    // monitor, restore it in case the OS parked it minimized, and
    // request focus. Without this, on some systems the window spawns
    // off-screen and only shows as a taskbar icon.
    SetWindowPosition((GetMonitorWidth(0) - screenWidth) / 2,
                      (GetMonitorHeight(0) - screenHeight) / 2);
    RestoreWindow();
    SetWindowFocused();

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
    // Real deltaTime flows into the simulation clock first; the scaled
    // simulation delta then drives the simulation. Future city systems
    // must consume simulation time, never GetFrameTime() directly.
    // Construction stays outside the simulation loop: player building
    // applies to the world immediately, even while paused.
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

    // Temporary A* debug test refreshes only when the world changes,
    // never every frame.
    if (pathTestDirty)
    {
        recomputePathTest();
        pathTestDirty = false;
    }
}

void Game::draw()
{
    ClearBackground(Color{ 20, 24, 34, 255 });

    camera.begin();
    drawWorld();
    drawBusStops();
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
    drawHighlight();
    drawPathTest();
    drawCommutePath();
    drawCitizens();
    drawVehicles();
    camera.end();

    // Polished Screen Space HUD
    drawTopRibbon();
    drawDemandMeters();
    drawBottomDock();
    if (showDashboard)
    {
        drawDashboardPanel();
    }
    drawRouteBanner();
    drawToastMessage();
    if (showSelfTestModal)
    {
        drawSelfTestModal();
    }
}

void Game::shutdown()
{
    simulation.shutdown();
    CloseWindow();
}

void Game::handleSimulationInput()
{
    // Simulation speeds live on F1-F4 so build selection on 1-5 is untouched.
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
    // Build-type selection also leaves demolition, bus stop, and route modes.
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

    const Vector2 mouse = GetMousePosition();
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();

    // Check if mouse clicked top ribbon interactive items
    if (mouse.y < 50)
    {
        // Pause toggle
        if (mouse.x >= 330 && mouse.x <= 372)
        {
            if (simulationClock.isPaused()) simulationClock.resume();
            else simulationClock.pause();
            return;
        }
        // Speed 1x, 2x, 4x, 8x
        if (mouse.x >= 378 && mouse.x <= 414) { simulationClock.setTimeScale(SimulationClock::NORMAL_SPEED); return; }
        if (mouse.x >= 418 && mouse.x <= 454) { simulationClock.setTimeScale(SimulationClock::FAST_SPEED); return; }
        if (mouse.x >= 458 && mouse.x <= 494) { simulationClock.setTimeScale(SimulationClock::VERY_FAST_SPEED); return; }
        if (mouse.x >= 498 && mouse.x <= 534) { simulationClock.setTimeScale(SimulationClock::EXTREMELY_FAST_SPEED); return; }

        // Overlay buttons at top-right
        if (mouse.x >= sw - 380 && mouse.x <= sw - 310) { pollutionOverlay = !pollutionOverlay; return; }
        if (mouse.x >= sw - 302 && mouse.x <= sw - 232) { landValueOverlay = !landValueOverlay; return; }
        if (mouse.x >= sw - 224 && mouse.x <= sw - 150) { housingOverlay = !housingOverlay; return; }
        if (mouse.x >= sw - 142 && mouse.x <= sw - 74)  { showDashboard = !showDashboard; return; }
        if (mouse.x >= sw - 66 && mouse.x <= sw - 12)   { showSelfTestModal = !showSelfTestModal; if (showSelfTestModal) { selfTest.run(world, simulation.getEconomy(), simulation); pathTestDirty = true; } return; }
        return;
    }

    // Check if mouse clicked bottom tool dock
    const int dockW = 840;
    const int dockH = 76;
    const int dockX = (sw - dockW) / 2;
    const int dockY = sh - 88;

    if (mouse.y >= dockY && mouse.y <= dockY + dockH && mouse.x >= dockX && mouse.x <= dockX + dockW)
    {
        const int btnW = 96;
        const int gap = 6;
        for (int i = 0; i < 8; ++i)
        {
            const int bx = dockX + 12 + i * (btnW + gap);
            if (mouse.x >= bx && mouse.x <= bx + btnW)
            {
                if (i == 0) { selectedBuildType = TileType::Road; demolishMode = false; busStopMode = false; routeMode = false; currentRouteStops.clear(); }
                else if (i == 1) { selectedBuildType = TileType::Residential; demolishMode = false; busStopMode = false; routeMode = false; currentRouteStops.clear(); }
                else if (i == 2) { selectedBuildType = TileType::Commercial; demolishMode = false; busStopMode = false; routeMode = false; currentRouteStops.clear(); }
                else if (i == 3) { selectedBuildType = TileType::Industrial; demolishMode = false; busStopMode = false; routeMode = false; currentRouteStops.clear(); }
                else if (i == 4) { selectedBuildType = TileType::Park; demolishMode = false; busStopMode = false; routeMode = false; currentRouteStops.clear(); }
                else if (i == 5) { busStopMode = !busStopMode; demolishMode = false; routeMode = false; currentRouteStops.clear(); }
                else if (i == 6) { routeMode = !routeMode; demolishMode = false; busStopMode = false; currentRouteStops.clear(); }
                else if (i == 7) { demolishMode = !demolishMode; busStopMode = false; routeMode = false; currentRouteStops.clear(); }
                return;
            }
        }
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
                    // Allow loop closing if it matches the first stop and we have >= 2 stops
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

void Game::drawWorld()
{
    const int tileSize = world.getTileSize();

    for (int y = 0; y < world.getHeight(); ++y)
    {
        for (int x = 0; x < world.getWidth(); ++x)
        {
            const Tile& tile = world.getTile(x, y);
            Color color = tileColor(tile.type);
            if (tile.type == TileType::Road)
            {
                color = congestedRoadColor(simulation.getCongestion().getCongestion(x, y));
            }
            DrawRectangle(x * tileSize, y * tileSize, tileSize, tileSize, color);
        }
    }

    const int gridWidth = world.getWidth() * tileSize;
    const int gridHeight = world.getHeight() * tileSize;

    for (int x = 0; x <= world.getWidth(); ++x)
    {
        DrawLine(x * tileSize, 0, x * tileSize, gridHeight, GRID_LINE);
    }

    for (int y = 0; y <= world.getHeight(); ++y)
    {
        DrawLine(0, y * tileSize, gridWidth, y * tileSize, GRID_LINE);
    }
}

void Game::drawHighlight()
{
    const urbania::TileCoordinate hovered = input.hovered();
    if (!hovered.valid)
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
        // Red preview on developed tiles, grey outline on Grass (nothing to demolish).
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
        // Occupied: cannot build here.
        DrawRectangleLinesEx(rect, 2.0f, BLOCKED_BORDER);
    }
    else if (!simulation.getEconomy().canAfford(selectedBuildType))
    {
        // Insufficient money: construction unavailable.
        DrawRectangleLinesEx(rect, 2.0f, UNAVAILABLE_BORDER);
    }
    else
    {
        // Valid Grass tile: subtle preview tinted with the selected type color.
        Color preview = tileColor(selectedBuildType);
        preview.a = 110;
        DrawRectangle(px, py, tileSize, tileSize, preview);
        DrawRectangle(px, py, tileSize, tileSize, PREVIEW_FILL);
        DrawRectangleLinesEx(rect, 2.0f, HIGHLIGHT_BORDER);
    }
}

void Game::recomputePathTest()
{
    // Temporary A* debug test: path between the first and last road
    // tiles in scan order. No path (fewer than two roads) is a valid
    // outcome, not an error.
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
    // Temporary debug visual: thin outline around each tile of the
    // test path so the route is visible while panning and zooming.
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
    // Temporary debug visual: small dots along the representative
    // commute route so it stays readable under the tile outlines.
    const int tileSize = world.getTileSize();
    for (const urbania::TileCoordinate& step : simulation.getCommuteSystem().getSampleRoute())
    {
        DrawCircle(step.x * tileSize + tileSize / 2, step.y * tileSize + tileSize / 2, 5.0f,
                   PATH_COMMUTE_DOT);
    }
}

void Game::drawCitizens()
{
    // Temporary citizen visual: one dot per citizen at the interpolated
    // world position between the last reached tile and the current
    // target, so movement reads smoothly instead of snapping per tile.
    const int tileSize = world.getTileSize();
    const float halfTile = static_cast<float>(tileSize) / 2.0f;

    for (const urbania::Citizen& citizen :
         simulation.getPopulation().getCitizens().getCitizens())
    {
        if (!citizen.currentTile.valid)
        {
            continue;
        }

        urbania::TileCoordinate target = citizen.currentTile;
        if (citizen.pathIndex >= 0 &&
            citizen.pathIndex < static_cast<int>(citizen.commutePath.size()))
        {
            target = citizen.commutePath[citizen.pathIndex];
        }

        const float x = (static_cast<float>(citizen.currentTile.x) +
                         (static_cast<float>(target.x - citizen.currentTile.x)) *
                             citizen.movementProgress) *
                            static_cast<float>(tileSize) +
                        halfTile;
        const float y = (static_cast<float>(citizen.currentTile.y) +
                         (static_cast<float>(target.y - citizen.currentTile.y)) *
                             citizen.movementProgress) *
                            static_cast<float>(tileSize) +
                        halfTile;

        DrawCircle(static_cast<int>(x), static_cast<int>(y), 6.0f, CITIZEN_DOT);
    }
}

void Game::drawVehicles()
{
    // Temporary vehicle visual: small rectangles at interpolated
    // positions, plus outlines along one representative route.
    const int tileSize = world.getTileSize();
    const float halfTile = static_cast<float>(tileSize) / 2.0f;

    for (const urbania::TileCoordinate& step : simulation.getTraffic().getRepresentativeRoute())
    {
        DrawRectangleLinesEx({ static_cast<float>(step.x * tileSize),
                               static_cast<float>(step.y * tileSize),
                               static_cast<float>(tileSize), static_cast<float>(tileSize) },
                             2.0f, PATH_VEHICLE_OUTLINE);
    }

    for (const urbania::Vehicle& vehicle : simulation.getTraffic().getVehicles())
    {
        if (!vehicle.active || vehicle.path.empty())
        {
            continue;
        }

        const int last = static_cast<int>(vehicle.path.size()) - 1;
        const int fromIndex = std::clamp(vehicle.pathIndex, 0, last);
        const int toIndex = std::min(fromIndex + 1, last);
        const urbania::TileCoordinate& from = vehicle.path[fromIndex];
        const urbania::TileCoordinate& to = vehicle.path[toIndex];

        const float x = (static_cast<float>(from.x) +
                         (static_cast<float>(to.x - from.x)) * vehicle.movementProgress) *
                            static_cast<float>(tileSize) +
                        halfTile;
        const float y = (static_cast<float>(from.y) +
                         (static_cast<float>(to.y - from.y)) * vehicle.movementProgress) *
                            static_cast<float>(tileSize) +
                        halfTile;

        DrawRectangle(static_cast<int>(x) - 5, static_cast<int>(y) - 5, 10, 10, VEHICLE_FILL);
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

        // Alpha scales with pollution level (up to 180 alpha at 100 pollution).
        const unsigned char alpha = static_cast<unsigned char>(
            std::clamp(pollution * 1.8f, 10.0f, 180.0f));
        // Brownish smog overlay tint
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

        // Visual overlay: higher land value is brighter green;
        // lower land value is darker/reddish tint.
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

        // Occupancy visualization:
        // High (>= 80%): solid vibrant green
        // Medium (40% - 70%): amber/yellow
        // Low (< 40%): light blue/cyan
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

void Game::drawBusStops()
{
    const int tileSize = world.getTileSize();
    for (const urbania::BusStop& stop : simulation.getTransit().getBusStops())
    {
        if (!stop.tile.valid)
        {
            continue;
        }
        const int px = stop.tile.x * tileSize;
        const int py = stop.tile.y * tileSize;

        // Bus stop sign marker on the road tile
        DrawRectangle(px + 4, py + 4, 12, 12, Color{ 255, 215, 0, 240 });
        DrawRectangleLines(px + 4, py + 4, 12, 12, Color{ 20, 50, 120, 255 });
        DrawText("B", px + 7, py + 5, 10, Color{ 20, 50, 120, 255 });
    }
}

void Game::drawBusRoutes()
{
    const int tileSize = world.getTileSize();
    const float halfTile = static_cast<float>(tileSize) / 2.0f;

    // Draw saved routes as colored connection lines
    const auto& routes = simulation.getTransit().getRoutes();
    const Color routeColors[] = {
        Color{ 180, 50, 220, 200 },  // Purple
        Color{ 40, 160, 220, 200 },  // Cyan
        Color{ 230, 120, 30, 200 },  // Orange
        Color{ 50, 200, 100, 200 },  // Green
        Color{ 220, 60, 100, 200 }   // Pink/Red
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

    // If currently editing a route, draw distinct bright lines between selected stops
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

        // Highlight selected stops with yellow circles
        for (size_t i = 0; i < currentRouteStops.size(); ++i)
        {
            const auto* stop = simulation.getTransit().getBusStopById(currentRouteStops[i]);
            if (stop != nullptr && stop->tile.valid)
            {
                DrawCircle(stop->tile.x * tileSize + halfTile, stop->tile.y * tileSize + halfTile,
                           7.0f, Color{ 255, 220, 0, 200 });
                DrawCircleLines(stop->tile.x * tileSize + halfTile, stop->tile.y * tileSize + halfTile,
                                7.0f, DARKBLUE);
            }
        }
    }
}

void Game::drawTopRibbon()
{
    const int sw = GetScreenWidth();
    const int barH = 50;

    // Dark glass ribbon background with subtle bottom border
    DrawRectangle(0, 0, sw, barH, Color{ 14, 18, 28, 245 });
    DrawRectangle(0, barH - 2, sw, 2, Color{ 38, 48, 70, 255 });

    // Logo & Brand
    DrawText("URBANIA", 18, 13, 22, Color{ 245, 250, 255, 255 });
    DrawRectangleRounded({ 122, 17, 44, 18 }, 0.4f, 4, Color{ 35, 65, 115, 255 });
    DrawText("CITY", 130, 20, 11, Color{ 120, 200, 255, 255 });

    // Time card (Calendar & Clock)
    DrawRectangleRounded({ 180, 8, 140, 34 }, 0.25f, 4, Color{ 22, 28, 44, 255 });
    DrawRectangleRoundedLines({ 180, 8, 140, 34 }, 0.25f, 4, Color{ 45, 58, 85, 255 });
    DrawText(TextFormat("Day %d • %02d:%02d", simulationClock.getDay(), simulationClock.getHour(),
                        simulationClock.getMinute()),
             192, 17, 16, Color{ 210, 225, 245, 255 });

    // Speed Controls
    const bool isPaused = simulationClock.isPaused();
    const float speed = simulationClock.getTimeScale();

    // Pause button
    const Color pauseBg = isPaused ? Color{ 220, 50, 50, 255 } : Color{ 26, 33, 50, 255 };
    const Color pauseText = isPaused ? WHITE : Color{ 150, 165, 190, 255 };
    DrawRectangleRounded({ 330, 10, 42, 30 }, 0.3f, 4, pauseBg);
    DrawRectangleRoundedLines({ 330, 10, 42, 30 }, 0.3f, 4, Color{ 45, 58, 85, 255 });
    DrawText("||", 346, 17, 15, pauseText);

    // Speed 1x, 2x, 4x, 8x
    const float speeds[] = { 1.0f, 2.0f, 4.0f, 8.0f };
    const char* speedLabels[] = { "1x", "2x", "4x", "8x" };
    for (int i = 0; i < 4; ++i)
    {
        const int bx = 378 + i * 40;
        const bool active = (!isPaused && speed == speeds[i]);
        const Color btnBg = active ? Color{ 0, 185, 245, 255 } : Color{ 26, 33, 50, 255 };
        const Color btnText = active ? Color{ 10, 20, 30, 255 } : Color{ 150, 165, 190, 255 };
        DrawRectangleRounded({ static_cast<float>(bx), 10, 36, 30 }, 0.3f, 4, btnBg);
        DrawRectangleRoundedLines({ static_cast<float>(bx), 10, 36, 30 }, 0.3f, 4,
                                  Color{ 45, 58, 85, 255 });
        DrawText(speedLabels[i], bx + 8, 17, 14, btnText);
    }

    // Money & Daily Cash Flow (Center)
    const int moneyX = 555;
    DrawRectangleRounded({ static_cast<float>(moneyX), 8, 185, 34 }, 0.25f, 4,
                         Color{ 22, 28, 44, 255 });
    DrawRectangleRoundedLines({ static_cast<float>(moneyX), 8, 185, 34 }, 0.25f, 4,
                              Color{ 45, 58, 85, 255 });
    DrawText(formatMoney(simulation.getEconomy().getMoney()).c_str(), moneyX + 10, 16, 16,
             Color{ 46, 204, 113, 255 });

    const int netInc = static_cast<int>(simulation.getEconomy().getNetIncome());
    if (netInc >= 0)
    {
        DrawText(TextFormat("+%s/d", formatMoney(netInc).c_str()), moneyX + 114, 18, 13,
                 Color{ 46, 204, 113, 220 });
    }
    else
    {
        DrawText(TextFormat("-%s/d", formatMoney(-netInc).c_str()), moneyX + 114, 18, 13,
                 Color{ 231, 76, 60, 220 });
    }

    // Population & Happiness
    const int popX = 750;
    DrawRectangleRounded({ static_cast<float>(popX), 8, 230, 34 }, 0.25f, 4,
                         Color{ 22, 28, 44, 255 });
    DrawRectangleRoundedLines({ static_cast<float>(popX), 8, 230, 34 }, 0.25f, 4,
                              Color{ 45, 58, 85, 255 });
    DrawText(TextFormat("Pop: %d", simulation.getPopulation().getTotalPopulation()), popX + 12, 16,
             16, WHITE);

    const float happy = simulation.getHappiness().getAverageHappiness();
    Color happyCol = Color{ 46, 204, 113, 255 }; // Green
    if (happy < 40.0f) happyCol = Color{ 231, 76, 60, 255 }; // Red
    else if (happy < 65.0f) happyCol = Color{ 241, 196, 15, 255 }; // Yellow

    DrawRectangleRounded({ static_cast<float>(popX + 110), 12, 110, 26 }, 0.3f, 4,
                         Color{ happyCol.r, happyCol.g, happyCol.b, 40 });
    DrawRectangleRoundedLines({ static_cast<float>(popX + 110), 12, 110, 26 }, 0.3f, 4,
                              happyCol);
    DrawText(TextFormat("%.1f%% Happy", happy), popX + 118, 17, 14, happyCol);

    // Right-side Overlay & Feature Pills
    const int pillY = 10;
    const int pillH = 30;

    // F5 Smog Overlay
    const Color smogBg = pollutionOverlay ? Color{ 210, 105, 30, 255 } : Color{ 26, 33, 50, 255 };
    const Color smogText = pollutionOverlay ? WHITE : Color{ 150, 165, 190, 255 };
    DrawRectangleRounded({ static_cast<float>(sw - 380), static_cast<float>(pillY), 70, static_cast<float>(pillH) },
                         0.3f, 4, smogBg);
    DrawRectangleRoundedLines({ static_cast<float>(sw - 380), static_cast<float>(pillY), 70, static_cast<float>(pillH) },
                              0.3f, 4, Color{ 45, 58, 85, 255 });
    DrawText("F5 Smog", sw - 373, pillY + 7, 13, smogText);

    // F6 Land Value Overlay
    const Color landBg = landValueOverlay ? Color{ 46, 204, 113, 255 } : Color{ 26, 33, 50, 255 };
    const Color landText = landValueOverlay ? Color{ 10, 25, 15, 255 } : Color{ 150, 165, 190, 255 };
    DrawRectangleRounded({ static_cast<float>(sw - 302), static_cast<float>(pillY), 70, static_cast<float>(pillH) },
                         0.3f, 4, landBg);
    DrawRectangleRoundedLines({ static_cast<float>(sw - 302), static_cast<float>(pillY), 70, static_cast<float>(pillH) },
                              0.3f, 4, Color{ 45, 58, 85, 255 });
    DrawText("F6 Land", sw - 295, pillY + 7, 13, landText);

    // F7 Housing Overlay
    const Color houseBg = housingOverlay ? Color{ 52, 152, 219, 255 } : Color{ 26, 33, 50, 255 };
    const Color houseText = housingOverlay ? WHITE : Color{ 150, 165, 190, 255 };
    DrawRectangleRounded({ static_cast<float>(sw - 224), static_cast<float>(pillY), 74, static_cast<float>(pillH) },
                         0.3f, 4, houseBg);
    DrawRectangleRoundedLines({ static_cast<float>(sw - 224), static_cast<float>(pillY), 74, static_cast<float>(pillH) },
                              0.3f, 4, Color{ 45, 58, 85, 255 });
    DrawText("F7 House", sw - 217, pillY + 7, 13, houseText);

    // TAB Dashboard Toggle
    const Color dashBg = showDashboard ? Color{ 142, 68, 173, 255 } : Color{ 26, 33, 50, 255 };
    const Color dashText = showDashboard ? WHITE : Color{ 150, 165, 190, 255 };
    DrawRectangleRounded({ static_cast<float>(sw - 142), static_cast<float>(pillY), 68, static_cast<float>(pillH) },
                         0.3f, 4, dashBg);
    DrawRectangleRoundedLines({ static_cast<float>(sw - 142), static_cast<float>(pillY), 68, static_cast<float>(pillH) },
                              0.3f, 4, Color{ 45, 58, 85, 255 });
    DrawText("TAB Info", sw - 135, pillY + 7, 13, dashText);

    // F9 SelfTest
    const Color testBg = showSelfTestModal ? Color{ 230, 126, 34, 255 } : Color{ 26, 33, 50, 255 };
    const Color testText = showSelfTestModal ? WHITE : Color{ 150, 165, 190, 255 };
    DrawRectangleRounded({ static_cast<float>(sw - 66), static_cast<float>(pillY), 54, static_cast<float>(pillH) },
                         0.3f, 4, testBg);
    DrawRectangleRoundedLines({ static_cast<float>(sw - 66), static_cast<float>(pillY), 54, static_cast<float>(pillH) },
                              0.3f, 4, Color{ 45, 58, 85, 255 });
    DrawText("F9 Test", sw - 60, pillY + 7, 13, testText);
}

void Game::drawDemandMeters()
{
    const int x = 16;
    const int y = 60;
    const int w = 152;
    const int h = 168;

    // Panel
    DrawRectangleRounded({ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                         0.1f, 4, Color{ 14, 18, 28, 235 });
    DrawRectangleRoundedLines({ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                              0.1f, 4, Color{ 40, 52, 75, 255 });

    DrawText("RCI DEMAND", x + 30, y + 10, 13, Color{ 200, 215, 235, 255 });
    DrawLine(x + 12, y + 28, x + w - 12, y + 28, Color{ 40, 52, 75, 255 });

    const int baseY = y + 92; // 0 line
    const int colW = 28;
    const int maxBarH = 46;

    // Draw baseline
    DrawLine(x + 18, baseY, x + w - 18, baseY, Color{ 70, 85, 115, 255 });

    // 1. Residential (R)
    const int rDem = simulation.getDemand().getResidentialDemand();
    const int rX = x + 24;
    const float rFrac = std::clamp(static_cast<float>(rDem) / 100.0f, -1.0f, 1.0f);
    const int rHeight = static_cast<int>(std::abs(rFrac) * maxBarH);
    if (rFrac > 0)
    {
        DrawRectangle(rX, baseY - rHeight, colW, rHeight, Color{ 46, 204, 113, 255 });
    }
    else if (rFrac < 0)
    {
        DrawRectangle(rX, baseY, colW, rHeight, Color{ 39, 174, 96, 200 });
    }
    DrawRectangleLines(rX, baseY - maxBarH, colW, maxBarH * 2, Color{ 35, 45, 65, 255 });
    DrawText("R", rX + 8, y + 144, 16, Color{ 46, 204, 113, 255 });
    DrawText(TextFormat("%+d", rDem), rX - 1, (rFrac >= 0) ? (baseY - rHeight - 13) : (baseY + rHeight + 2), 10,
             Color{ 200, 240, 210, 255 });

    // 2. Commercial (C)
    const int cDem = simulation.getDemand().getCommercialDemand();
    const int cX = x + 62;
    const float cFrac = std::clamp(static_cast<float>(cDem) / 100.0f, -1.0f, 1.0f);
    const int cHeight = static_cast<int>(std::abs(cFrac) * maxBarH);
    if (cFrac > 0)
    {
        DrawRectangle(cX, baseY - cHeight, colW, cHeight, Color{ 52, 152, 219, 255 });
    }
    else if (cFrac < 0)
    {
        DrawRectangle(cX, baseY, colW, cHeight, Color{ 41, 128, 185, 200 });
    }
    DrawRectangleLines(cX, baseY - maxBarH, colW, maxBarH * 2, Color{ 35, 45, 65, 255 });
    DrawText("C", cX + 8, y + 144, 16, Color{ 52, 152, 219, 255 });
    DrawText(TextFormat("%+d", cDem), cX - 1, (cFrac >= 0) ? (baseY - cHeight - 13) : (baseY + cHeight + 2), 10,
             Color{ 200, 230, 255, 255 });

    // 3. Industrial (I)
    const int iDem = simulation.getDemand().getIndustrialDemand();
    const int iX = x + 100;
    const float iFrac = std::clamp(static_cast<float>(iDem) / 100.0f, -1.0f, 1.0f);
    const int iHeight = static_cast<int>(std::abs(iFrac) * maxBarH);
    if (iFrac > 0)
    {
        DrawRectangle(iX, baseY - iHeight, colW, iHeight, Color{ 230, 126, 34, 255 });
    }
    else if (iFrac < 0)
    {
        DrawRectangle(iX, baseY, colW, iHeight, Color{ 211, 84, 0, 200 });
    }
    DrawRectangleLines(iX, baseY - maxBarH, colW, maxBarH * 2, Color{ 35, 45, 65, 255 });
    DrawText("I", iX + 9, y + 144, 16, Color{ 230, 126, 34, 255 });
    DrawText(TextFormat("%+d", iDem), iX - 1, (iFrac >= 0) ? (baseY - iHeight - 13) : (baseY + iHeight + 2), 10,
             Color{ 255, 230, 200, 255 });
}

void Game::drawBottomDock()
{
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();
    const int dockW = 840;
    const int dockH = 76;
    const int dockX = (sw - dockW) / 2;
    const int dockY = sh - 88;

    // Background Dock Glass
    DrawRectangleRounded({ static_cast<float>(dockX), static_cast<float>(dockY),
                           static_cast<float>(dockW), static_cast<float>(dockH) },
                         0.2f, 4, Color{ 14, 18, 28, 245 });
    DrawRectangleRoundedLines({ static_cast<float>(dockX), static_cast<float>(dockY),
                                static_cast<float>(dockW), static_cast<float>(dockH) },
                              0.2f, 4, Color{ 45, 60, 90, 255 });

    struct ToolInfo {
        const char* key;
        const char* name;
        const char* price;
        Color swatchColor;
        bool isSelected;
    };

    const ToolInfo tools[8] = {
        { "[1]", "Road", "Rs. 100", Color{ 120, 120, 120, 255 }, !demolishMode && !busStopMode && !routeMode && selectedBuildType == TileType::Road },
        { "[2]", "Resi", "Rs. 2k", Color{ 80, 150, 255, 255 }, !demolishMode && !busStopMode && !routeMode && selectedBuildType == TileType::Residential },
        { "[3]", "Comm", "Rs. 5k", Color{ 255, 170, 30, 255 }, !demolishMode && !busStopMode && !routeMode && selectedBuildType == TileType::Commercial },
        { "[4]", "Ind", "Rs. 10k", Color{ 190, 70, 70, 255 }, !demolishMode && !busStopMode && !routeMode && selectedBuildType == TileType::Industrial },
        { "[5]", "Park", "Rs. 1k", Color{ 35, 140, 60, 255 }, !demolishMode && !busStopMode && !routeMode && selectedBuildType == TileType::Park },
        { "[B]", "Bus Stop", "Rs. 500", Color{ 255, 215, 0, 255 }, busStopMode },
        { "[R]", "Route", "Transit", Color{ 180, 50, 220, 255 }, routeMode },
        { "[D]", "Demolish", "Clear", Color{ 220, 50, 50, 255 }, demolishMode }
    };

    const int btnW = 96;
    const int btnH = 60;
    const int gap = 6;
    const Vector2 mouse = GetMousePosition();

    for (int i = 0; i < 8; ++i)
    {
        const int bx = dockX + 12 + i * (btnW + gap);
        const int by = dockY + 8;
        const bool hovered = (mouse.x >= bx && mouse.x <= bx + btnW && mouse.y >= by && mouse.y <= by + btnH);
        const bool selected = tools[i].isSelected;

        Color btnBg = selected ? Color{ 35, 48, 75, 255 } : (hovered ? Color{ 25, 34, 52, 255 } : Color{ 18, 24, 38, 255 });
        Color borderColor = selected ? Color{ 255, 215, 0, 255 } : (hovered ? Color{ 100, 130, 180, 255 } : Color{ 35, 48, 70, 255 });

        DrawRectangleRounded({ static_cast<float>(bx), static_cast<float>(by),
                               static_cast<float>(btnW), static_cast<float>(btnH) },
                             0.2f, 4, btnBg);
        DrawRectangleRoundedLines({ static_cast<float>(bx), static_cast<float>(by),
                                    static_cast<float>(btnW), static_cast<float>(btnH) },
                                  0.2f, 4, borderColor);

        // Color Swatch Badge
        DrawRectangleRounded({ static_cast<float>(bx + 8), static_cast<float>(by + 8), 16, 16 },
                             0.3f, 2, tools[i].swatchColor);

        // Hotkey tag
        DrawText(tools[i].key, bx + 30, by + 8, 12, Color{ 130, 155, 195, 255 });

        // Name
        DrawText(tools[i].name, bx + 8, by + 28, 13, selected ? Color{ 255, 255, 255, 255 } : Color{ 210, 220, 235, 255 });

        // Price
        DrawText(tools[i].price, bx + 8, by + 43, 11, Color{ 46, 204, 113, 220 });
    }
}

void Game::drawDashboardPanel()
{
    const int sw = GetScreenWidth();
    const int w = 340;
    const int x = sw - w - 16;
    const int y = 60;
    const int h = 570;

    // Background Card
    DrawRectangleRounded({ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                         0.05f, 4, Color{ 14, 18, 28, 240 });
    DrawRectangleRoundedLines({ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                              0.05f, 4, Color{ 40, 52, 75, 255 });

    int cy = y + 12;

    // Header
    DrawText("CITY DASHBOARD", x + 16, cy, 14, Color{ 200, 215, 240, 255 });
    DrawText("[TAB to Hide]", x + w - 95, cy + 2, 11, Color{ 120, 140, 175, 255 });
    cy += 22;
    DrawLine(x + 12, cy, x + w - 12, cy, Color{ 35, 48, 70, 255 });
    cy += 10;

    // SECTION 1: TILE INSPECTOR
    const urbania::TileCoordinate hovered = input.hovered();
    DrawText("INSPECTOR", x + 16, cy, 11, Color{ 0, 180, 240, 255 });
    cy += 16;

    if (hovered.valid)
    {
        const Tile& t = world.getTile(hovered.x, hovered.y);
        DrawText(TextFormat("Tile: (%d, %d)", hovered.x, hovered.y), x + 16, cy, 13, WHITE);

        // Zone Badge
        DrawRectangleRounded({ static_cast<float>(x + 130), static_cast<float>(cy - 2), 90, 18 }, 0.3f, 4, tileColor(t.type));
        DrawText(tileTypeName(t.type), x + 138, cy + 1, 11, Color{ 10, 20, 30, 255 });
        cy += 20;

        // Land Value
        const float lv = simulation.getLandValue().getLandValue(hovered.x, hovered.y);
        DrawText(TextFormat("Land Value: %.1f / 100", lv), x + 16, cy, 12, Color{ 190, 205, 225, 255 });
        // Mini bar
        DrawRectangle(x + 160, cy + 3, 140, 8, Color{ 30, 40, 60, 255 });
        DrawRectangle(x + 160, cy + 3, static_cast<int>(lv * 1.4f), 8, (lv >= 50.0f) ? Color{ 46, 204, 113, 255 } : Color{ 231, 76, 60, 255 });
        cy += 18;

        if (t.type == TileType::Residential)
        {
            const int res = simulation.getPopulation().getResidentsAt(hovered.x, hovered.y);
            DrawText(TextFormat("Residents: %d / %d", res, urbania::Housing::CAPACITY_PER_TILE), x + 16, cy, 12, Color{ 190, 205, 225, 255 });
            DrawRectangle(x + 160, cy + 3, 140, 8, Color{ 30, 40, 60, 255 });
            DrawRectangle(x + 160, cy + 3, static_cast<int>((res / 10.0f) * 140), 8, Color{ 80, 150, 255, 255 });
            cy += 18;
        }
        else if (t.type == TileType::Road)
        {
            const int vCount = simulation.getCongestion().getVehicleCount(hovered.x, hovered.y);
            const float cong = simulation.getCongestion().getCongestion(hovered.x, hovered.y);
            DrawText(TextFormat("Traffic: %d / 5 (%.1fx)", vCount, cong), x + 16, cy, 12, Color{ 190, 205, 225, 255 });
            DrawRectangle(x + 160, cy + 3, 140, 8, Color{ 30, 40, 60, 255 });
            DrawRectangle(x + 160, cy + 3, std::min(140, static_cast<int>(cong * 140)), 8, (cong > 1.0f) ? Color{ 231, 76, 60, 255 } : Color{ 241, 196, 15, 255 });
            cy += 18;
        }

        if (simulation.getTransit().hasBusStop(hovered))
        {
            const auto* stop = simulation.getTransit().getBusStop(hovered);
            if (stop != nullptr)
            {
                DrawText(TextFormat("Transit Stop: Bus Stop #%d", stop->id), x + 16, cy, 12, Color{ 255, 215, 0, 255 });
                cy += 18;
            }
        }

        const float p = simulation.getPollution().getPollution(hovered.x, hovered.y);
        if (p > 0.001f)
        {
            DrawText(TextFormat("Pollution: %.1f ppm", p), x + 16, cy, 12, Color{ 230, 126, 34, 255 });
            cy += 18;
        }
    }
    else
    {
        DrawText("Hover over city tiles to inspect.", x + 16, cy, 12, Color{ 130, 145, 170, 255 });
        cy += 24;
    }

    cy += 6;
    DrawLine(x + 12, cy, x + w - 12, cy, Color{ 35, 48, 70, 255 });
    cy += 10;

    // SECTION 2: ECONOMY & BUDGET
    DrawText("FINANCES", x + 16, cy, 11, Color{ 46, 204, 113, 255 });
    cy += 16;
    DrawText(TextFormat("Daily Taxes: %s", formatMoney(static_cast<int>(simulation.getEconomy().getTaxIncome())).c_str()), x + 16, cy, 12, Color{ 190, 205, 225, 255 });
    DrawText(TextFormat("Upkeep: %s", formatMoney(static_cast<int>(simulation.getEconomy().getMaintenanceCost())).c_str()), x + 175, cy, 12, Color{ 190, 205, 225, 255 });
    cy += 18;
    const int netFlow = static_cast<int>(simulation.getEconomy().getNetIncome());
    DrawText(TextFormat("Net Cash Flow: %s%s / day", (netFlow >= 0 ? "+" : "-"), formatMoney(std::abs(netFlow)).c_str()),
             x + 16, cy, 13, (netFlow >= 0 ? Color{ 46, 204, 113, 255 } : Color{ 231, 76, 60, 255 }));
    cy += 24;

    DrawLine(x + 12, cy, x + w - 12, cy, Color{ 35, 48, 70, 255 });
    cy += 10;

    // SECTION 3: HOUSING & WORKFORCE
    DrawText("HOUSING & JOBS", x + 16, cy, 11, Color{ 52, 152, 219, 255 });
    cy += 16;
    DrawText(TextFormat("Housing: %d / %d (%.0f%%)", simulation.getHousing().getTotalResidents(), simulation.getHousing().getTotalCapacity(), simulation.getHousing().getOccupancyRatio() * 100.0f), x + 16, cy, 12, Color{ 190, 205, 225, 255 });
    DrawText(TextFormat("Pressure: %+d", simulation.getHousing().getHousingPressure()), x + 235, cy, 12, (simulation.getHousing().getHousingPressure() > 0 ? Color{ 241, 196, 15, 255 } : Color{ 140, 160, 190, 255 }));
    cy += 18;
    DrawText(TextFormat("Workplace Jobs: %d / %d filled", simulation.getEmployment().getOccupiedJobs(), simulation.getEmployment().getTotalJobs()), x + 16, cy, 12, Color{ 190, 205, 225, 255 });
    cy += 18;
    DrawText(TextFormat("Workforce: %d employed • %d unemployed", simulation.getEmployment().getEmployedCitizens(), simulation.getEmployment().getUnemployedCitizens()), x + 16, cy, 12, Color{ 190, 205, 225, 255 });
    cy += 24;

    DrawLine(x + 12, cy, x + w - 12, cy, Color{ 35, 48, 70, 255 });
    cy += 10;

    // SECTION 4: MOBILITY & ENVIRONMENT
    DrawText("TRANSIT & QUALITY OF LIFE", x + 16, cy, 11, Color{ 230, 126, 34, 255 });
    cy += 16;
    DrawText(TextFormat("Roads: %d nodes • Traffic: %d cars", simulation.getRoadNetwork().getNodeCount(), simulation.getTraffic().getActiveVehicleCount()), x + 16, cy, 12, Color{ 190, 205, 225, 255 });
    cy += 18;
    DrawText(TextFormat("Transit: %d bus stops • %d routes", simulation.getTransit().getBusStopCount(), simulation.getTransit().getRouteCount()), x + 16, cy, 12, Color{ 190, 205, 225, 255 });
    cy += 18;
    DrawText(TextFormat("Commuters: %d routed • %d unrouted", simulation.getCommuteSystem().getRoutedCitizens(), simulation.getCommuteSystem().getUnroutedCitizens()), x + 16, cy, 12, Color{ 190, 205, 225, 255 });
    cy += 18;
    DrawText(TextFormat("Air Quality: %.1f avg ppm • Avg Land: %.1f", simulation.getPollution().getAveragePollution(), simulation.getLandValue().getAverageLandValue()), x + 16, cy, 12, Color{ 190, 205, 225, 255 });
}

void Game::drawRouteBanner()
{
    if (!routeMode)
    {
        return;
    }

    const int sw = GetScreenWidth();
    const int w = 540;
    const int h = 54;
    const int x = (sw - w) / 2;
    const int y = 58;

    DrawRectangleRounded({ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                         0.25f, 4, Color{ 45, 20, 60, 240 });
    DrawRectangleRoundedLines({ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                              0.25f, 4, Color{ 180, 50, 220, 255 });

    DrawText("BUS ROUTE BUILDER", x + 16, y + 10, 14, Color{ 255, 220, 40, 255 });
    DrawText("[ENTER] Save  [ESC] Cancel  [Shift+R] Delete Latest", x + 190, y + 11, 12, Color{ 220, 200, 240, 255 });

    std::string seq = "Stops: ";
    if (currentRouteStops.empty())
    {
        seq += "Click bus stops on map to connect...";
    }
    else
    {
        for (size_t i = 0; i < currentRouteStops.size(); ++i)
        {
            if (i > 0) seq += " -> ";
            seq += "Stop #" + std::to_string(currentRouteStops[i]);
        }
        seq += " (" + std::to_string(currentRouteStops.size()) + " total)";
    }
    DrawText(seq.c_str(), x + 16, y + 30, 12, Color{ 245, 235, 255, 255 });
}

void Game::drawToastMessage()
{
    if (transitMessage.empty() || transitMessageTimer <= 0.0f)
    {
        return;
    }

    const int sw = GetScreenWidth();
    const int textW = MeasureText(transitMessage.c_str(), 14);
    const int w = textW + 40;
    const int h = 36;
    const int x = (sw - w) / 2;
    const int y = routeMode ? 120 : 60;

    DrawRectangleRounded({ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                         0.3f, 4, Color{ 20, 28, 45, 240 });
    DrawRectangleRoundedLines({ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                              0.3f, 4, Color{ 0, 180, 240, 255 });

    DrawText(transitMessage.c_str(), x + 20, y + 11, 14, Color{ 240, 245, 255, 255 });
}

void Game::drawSelfTestModal()
{
    if (!selfTest.hasRun())
    {
        return;
    }

    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();
    const int w = 480;
    const int h = 520;
    const int x = (sw - w) / 2;
    const int y = (sh - h) / 2;

    // Dark backdrop overlay
    DrawRectangle(0, 0, sw, sh, Color{ 0, 0, 0, 120 });

    // Dialog card
    DrawRectangleRounded({ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                         0.08f, 4, Color{ 16, 22, 34, 250 });
    DrawRectangleRoundedLines({ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                              0.08f, 4, Color{ 60, 80, 120, 255 });

    const int passed = selfTest.getPassed();
    const int total = selfTest.getTotal();
    const bool allPassed = (total > 0 && passed == total);

    // Banner
    Color headerBg = allPassed ? Color{ 39, 174, 96, 255 } : Color{ 192, 57, 43, 255 };
    DrawRectangleRounded({ static_cast<float>(x + 12), static_cast<float>(y + 12), static_cast<float>(w - 24), 40 }, 0.2f, 4, headerBg);
    DrawText(TextFormat("SELF-TEST SUITE: %d / %d PASSED", passed, total), x + 24, y + 23, 16, WHITE);

    // Results scroll list
    int listY = y + 66;
    int count = 0;
    for (const std::string& line : selfTest.getResults())
    {
        if (count >= 16)
        {
            DrawText("... (and more)", x + 24, listY, 12, Color{ 140, 160, 190, 255 });
            break;
        }

        const bool isOk = line.rfind("ok", 0) == 0;
        const bool isSkip = line.rfind("SKIP", 0) == 0;

        Color badgeColor = isOk ? Color{ 46, 204, 113, 255 } : (isSkip ? Color{ 241, 196, 15, 255 } : Color{ 231, 76, 60, 255 });
        DrawRectangleRounded({ static_cast<float>(x + 24), static_cast<float>(listY), 36, 18 }, 0.3f, 4, badgeColor);
        DrawText(isOk ? "PASS" : (isSkip ? "SKIP" : "FAIL"), x + 28, listY + 3, 10, Color{ 10, 20, 30, 255 });

        DrawText(line.c_str(), x + 68, listY + 2, 12, Color{ 215, 225, 240, 255 });
        listY += 24;
        ++count;
    }

    // Close button footer
    DrawRectangleRounded({ static_cast<float>(x + w / 2 - 60), static_cast<float>(y + h - 42), 120, 28 }, 0.3f, 4, Color{ 45, 60, 90, 255 });
    DrawText("Close (ESC)", x + w / 2 - 38, y + h - 35, 13, WHITE);

    if (IsKeyPressed(KEY_ESCAPE) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && GetMousePosition().y >= y + h - 42 && GetMousePosition().y <= y + h - 14 && GetMousePosition().x >= x + w/2 - 60 && GetMousePosition().x <= x + w/2 + 60))
    {
        showSelfTestModal = false;
    }
}

