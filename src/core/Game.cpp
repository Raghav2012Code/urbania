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
    ClearBackground(RAYWHITE);

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

    drawDebugText();
    drawSelfTest();
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
    else if (IsKeyPressed(KEY_F9))
    {
        // Development self-test: scripted checks against the live
        // systems. Uses direct calls and fixed deltas, never the OS
        // mouse, so results are cursor-independent.
        selfTest.run(world, simulation.getEconomy(), simulation);
        pathTestDirty = true;
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
            simulation.getTransit().removeBusStop(hovered);
        }
        else
        {
            simulation.getTransit().addBusStop(world, hovered, simulation.getEconomy());
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
    const TileType oldType = tile.type;

    bool changed = false;
    if (demolishMode)
    {
        changed = simulation.getEconomy().tryDemolish(tile);
    }
    else
    {
        changed = simulation.getEconomy().tryBuild(tile, selectedBuildType);
    }

    // Refresh the road graph only when a tile changes to or from Road.
    // Any successful edit also refreshes the temporary path test and syncs transit.
    if (changed && (oldType == TileType::Road || tile.type == TileType::Road))
    {
        simulation.getRoadNetwork().rebuild(world);
        simulation.getTransit().syncWithWorld(world);
    }
    if (changed)
    {
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

void Game::drawDebugText()
{
    const urbania::TileCoordinate hovered = input.hovered();
    int y = 10;
    const int step = 26;

    if (!transitMessage.empty())
    {
        DrawText(transitMessage.c_str(), 10, y, 20, { 220, 110, 20, 255 });
        y += step;
    }

    if (hovered.valid)
    {
        DrawText(TextFormat("Tile: (%d, %d)", hovered.x, hovered.y), 10, y, 20, DARKGRAY);
        y += step;
        DrawText(TextFormat("Land Value: %.1f",
                            simulation.getLandValue().getLandValue(hovered.x, hovered.y)),
                 10, y, 20, DARKGRAY);
        y += step;
        if (world.getTile(hovered.x, hovered.y).type == TileType::Residential)
        {
            DrawText(TextFormat("Residents: %d / %d",
                                simulation.getPopulation().getResidentsAt(hovered.x, hovered.y),
                                urbania::Housing::CAPACITY_PER_TILE),
                     10, y, 20, DARKGRAY);
            y += step;
            DrawText(TextFormat("Residential Value: %.1f",
                                simulation.getHousing().getResidentialValue(hovered.x, hovered.y)),
                     10, y, 20, DARKGRAY);
            y += step;
        }
        if (simulation.getTransit().hasBusStop(hovered))
        {
            const urbania::BusStop* stop = simulation.getTransit().getBusStop(hovered);
            if (stop != nullptr)
            {
                DrawText(TextFormat("Bus Stop ID: %d", stop->id), 10, y, 20, DARKBLUE);
                y += step;
            }
        }
        if (pollutionOverlay)
        {
            DrawText(TextFormat("Tile Pollution: %.1f",
                                simulation.getPollution().getPollution(hovered.x, hovered.y)),
                     10, y, 20, DARKGRAY);
            y += step;
        }
    }
    else
    {
        DrawText("Tile: Outside world", 10, y, 20, DARKGRAY);
        y += step;
    }

    if (demolishMode)
    {
        DrawText("Mode: DEMOLISH (D to exit)", 10, y, 20, { 200, 40, 40, 255 });
        y += step;
    }
    else if (busStopMode)
    {
        DrawText("Mode: BUS STOP (B to exit)", 10, y, 20, { 40, 120, 220, 255 });
        y += step;
        DrawText(TextFormat("Cost: %s (Shift+Click to remove)",
                            formatMoney(urbania::Transit::BUS_STOP_COST).c_str()),
                 10, y, 20, DARKGRAY);
        y += step;
    }
    else if (routeMode)
    {
        DrawText("Route Mode", 10, y, 20, { 180, 50, 220, 255 });
        y += step;
        DrawText(TextFormat("Stops: %d", static_cast<int>(currentRouteStops.size())), 10, y,
                 20, DARKGRAY);
        y += step;

        std::string routeSeq = "Current Route: ";
        if (currentRouteStops.empty())
        {
            routeSeq += "(none - click stops, Enter save, Esc cancel)";
        }
        else
        {
            for (size_t i = 0; i < currentRouteStops.size(); ++i)
            {
                if (i > 0)
                {
                    routeSeq += " -> ";
                }
                routeSeq += std::to_string(currentRouteStops[i]);
            }
        }
        DrawText(routeSeq.c_str(), 10, y, 20, DARKGRAY);
        y += step;
    }
    else
    {
        DrawText(TextFormat("Build: %s", tileTypeName(selectedBuildType)), 10, y, 20, DARKGRAY);
        y += step;
        DrawText(TextFormat("Cost: %s", formatMoney(Economy::getCost(selectedBuildType)).c_str()), 10,
                 y, 20, DARKGRAY);
        y += step;

        if (selectedBuildType == TileType::Residential)
        {
            DrawText(TextFormat("Demand: %+d", simulation.getDemand().getResidentialDemand()), 10,
                     y, 20, DARKGRAY);
            y += step;
        }
        else if (selectedBuildType == TileType::Commercial)
        {
            DrawText(TextFormat("Demand: %+d", simulation.getDemand().getCommercialDemand()), 10,
                     y, 20, DARKGRAY);
            y += step;
        }
        else if (selectedBuildType == TileType::Industrial)
        {
            DrawText(TextFormat("Demand: %+d", simulation.getDemand().getIndustrialDemand()), 10,
                     y, 20, DARKGRAY);
            y += step;
        }
    }

    DrawText(TextFormat("Money: %s", formatMoney(simulation.getEconomy().getMoney()).c_str()), 10,
             y, 20, DARKGRAY);
    y += step;
    DrawText(TextFormat("Daily Income: %s",
                        formatMoney(static_cast<int>(simulation.getEconomy().getNetIncome())).c_str()),
             10, y, 20, DARKGRAY);
    y += step;
    DrawText(TextFormat("Tax: %s",
                        formatMoney(static_cast<int>(simulation.getEconomy().getTaxIncome())).c_str()),
             10, y, 20, DARKGRAY);
    y += step;
    DrawText(TextFormat("Maintenance: %s",
                        formatMoney(static_cast<int>(simulation.getEconomy().getMaintenanceCost()))
                            .c_str()),
             10, y, 20, DARKGRAY);
    y += step;

    // Demand overview
    DrawText("Demand", 10, y, 20, DARKGRAY);
    y += step;
    DrawText(TextFormat("  Residential: %+d", simulation.getDemand().getResidentialDemand()), 10,
             y, 20, DARKGRAY);
    y += step;
    DrawText(TextFormat("  Commercial: %+d", simulation.getDemand().getCommercialDemand()), 10,
             y, 20, DARKGRAY);
    y += step;
    DrawText(TextFormat("  Industrial: %+d", simulation.getDemand().getIndustrialDemand()), 10,
             y, 20, DARKGRAY);
    y += step;

    if (pollutionOverlay)
    {
        DrawText("Pollution", 10, y, 20, DARKGRAY);
        y += step;
        DrawText(TextFormat("  Average: %.1f", simulation.getPollution().getAveragePollution()),
                 10, y, 20, DARKGRAY);
        y += step;
        DrawText(TextFormat("  Maximum: %.1f", simulation.getPollution().getMaxPollution()),
                 10, y, 20, DARKGRAY);
        y += step;
    }

    if (landValueOverlay)
    {
        DrawText("Land Value Overlay (F6)", 10, y, 20, { 50, 180, 80, 255 });
        y += step;
    }
    if (housingOverlay)
    {
        DrawText("Housing Overlay (F7)", 10, y, 20, { 30, 160, 60, 255 });
        y += step;
    }
    DrawText(TextFormat("Average Land Value: %.1f",
                        simulation.getLandValue().getAverageLandValue()),
             10, y, 20, DARKGRAY);
    y += step;

    DrawText(TextFormat("Day %d - %02d:%02d", simulationClock.getDay(), simulationClock.getHour(),
                        simulationClock.getMinute()),
             10, y, 20, DARKGRAY);
    y += step;

    if (simulationClock.isPaused())
    {
        DrawText("PAUSED", 10, y, 20, { 200, 40, 40, 255 });
    }
    else
    {
        DrawText(TextFormat("Speed: %dx", static_cast<int>(simulationClock.getTimeScale())), 10, y,
             20, DARKGRAY);
    }
    y += step;

    DrawText(TextFormat("Simulated: %.1fs", simulation.getElapsedSimulationSeconds()), 10, y, 20,
             DARKGRAY);
    y += step;
    DrawText(TextFormat("Population: %d", simulation.getPopulation().getTotalPopulation()), 10, y,
             20, DARKGRAY);
    y += step;
    DrawText(TextFormat("Happiness: %.1f", simulation.getHappiness().getAverageHappiness()), 10, y,
             20, DARKGRAY);
    y += step;
    DrawText("Housing", 10, y, 20, DARKGRAY);
    y += step;
    DrawText(TextFormat("  Capacity: %d", simulation.getHousing().getTotalCapacity()), 10, y,
             20, DARKGRAY);
    y += step;
    DrawText(TextFormat("  Residents: %d", simulation.getHousing().getTotalResidents()), 10, y,
             20, DARKGRAY);
    y += step;
    DrawText(TextFormat("  Occupancy: %.1f%%",
                        simulation.getHousing().getOccupancyRatio() * 100.0f),
             10, y, 20, DARKGRAY);
    y += step;
    DrawText(TextFormat("  Housing Pressure: %+d", simulation.getHousing().getHousingPressure()),
             10, y, 20, DARKGRAY);
    y += step;
    DrawText(TextFormat("Jobs: %d / %d", simulation.getEmployment().getOccupiedJobs(),
                        simulation.getEmployment().getTotalJobs()),
             10, y, 20, DARKGRAY);
    y += step;
    DrawText(TextFormat("Employment: %d / %d", simulation.getEmployment().getEmployedCitizens(),
                        simulation.getPopulation().getTotalPopulation()),
             10, y, 20, DARKGRAY);
    y += step;
    DrawText(TextFormat("Unemployed: %d", simulation.getEmployment().getUnemployedCitizens()), 10,
             y, 20, DARKGRAY);
    y += step;
    DrawText(TextFormat("Road Nodes: %d", simulation.getRoadNetwork().getNodeCount()), 10, y, 20,
             DARKGRAY);
    y += step;

    if (pathTest.empty())
    {
        DrawText("Path Test: No Path", 10, y, 20, DARKGRAY);
    }
    else
    {
        DrawText(TextFormat("Path Test Length: %d", static_cast<int>(pathTest.size())), 10, y,
                 20, DARKGRAY);
    }
    y += step;

    DrawText(TextFormat("Commute Routes: %d", simulation.getCommuteSystem().getRoutedCitizens()),
             10, y, 20, DARKGRAY);
    y += step;
    DrawText(TextFormat("No Route: %d", simulation.getCommuteSystem().getUnroutedCitizens()), 10,
             y, 20, DARKGRAY);
    y += step;
    DrawText(TextFormat("Route Length: %d",
                        static_cast<int>(simulation.getCommuteSystem().getSampleRoute().size())),
             10, y, 20, DARKGRAY);
    y += step;

    DrawText(TextFormat("Moving Citizens: %d",
                        simulation.getCitizenMovement().getMovingCitizens()),
             10, y, 20, DARKGRAY);
    y += step;

    const urbania::Citizen* representative = nullptr;
    for (const urbania::Citizen& citizen :
         simulation.getPopulation().getCitizens().getCitizens())
    {
        if (!citizen.commutePath.empty())
        {
            representative = &citizen;
            break;
        }
    }

    if (representative != nullptr)
    {
        DrawText(TextFormat("Citizen %d: %d/%d (Happy: %.1f)", representative->id,
                            representative->pathIndex,
                            static_cast<int>(representative->commutePath.size()),
                            representative->happiness),
                 10, y, 20, DARKGRAY);
    }
    else
    {
        DrawText("Citizen: --", 10, y, 20, DARKGRAY);
    }
    y += step;

    DrawText(TextFormat("Vehicles: %d", simulation.getTraffic().getVehicleCount()), 10, y,
             20, DARKGRAY);
    y += step;
    DrawText(TextFormat("Active Vehicles: %d", simulation.getTraffic().getActiveVehicleCount()),
             10, y, 20, DARKGRAY);
    y += step;
    DrawText(TextFormat("Roads: %d", simulation.getRoadNetwork().getNodeCount()), 10, y,
             20, DARKGRAY);
    y += step;
    DrawText(TextFormat("Bus Stops: %d", simulation.getTransit().getBusStopCount()), 10, y,
             20, DARKGRAY);
    y += step;
    DrawText(TextFormat("Bus Routes: %d", simulation.getTransit().getRouteCount()), 10, y,
             20, DARKGRAY);
    y += step;
    DrawText(TextFormat("Congested Roads: %d", simulation.getCongestion().getCongestedRoadCount()),
             10, y, 20, DARKGRAY);
    y += step;
    DrawText(TextFormat("Max Congestion: %.1f", simulation.getCongestion().getMaxCongestion()), 10,
             y, 20, DARKGRAY);
    y += step;

    if (hovered.valid &&
        simulation.getRoadNetwork().isRoad(hovered.x, hovered.y))
    {
        DrawText(TextFormat("Road Neighbors: %d",
                            static_cast<int>(simulation.getRoadNetwork()
                                                 .getNeighbors(hovered)
                                                 .size())),
                 10, y, 20, DARKGRAY);
        y += step;
        DrawText(TextFormat("Vehicles: %d / Capacity: %d",
                            simulation.getCongestion().getVehicleCount(hovered.x, hovered.y),
                            urbania::Congestion::getCapacity()),
                 10, y, 20, DARKGRAY);
        y += step;
        DrawText(TextFormat("Congestion: %.1f",
                            simulation.getCongestion().getCongestion(hovered.x, hovered.y)),
                 10, y, 20, DARKGRAY);
        y += step;
        DrawText("Keys: 1-5 select, B bus stop, R route (Shift+R del), D demolish, Space pause, F1-F4 speed, F5-F7 overlays, F9 self-test", 10, y,
                 20, DARKGRAY);
    }
    else
    {
        DrawText("Keys: 1-5 select, B bus stop, R route (Shift+R del), D demolish, Space pause, F1-F4 speed, F5-F7 overlays, F9 self-test", 10, y,
                 20, DARKGRAY);
    }
}

void Game::drawSelfTest()
{
    if (!selfTest.hasRun())
    {
        return;
    }

    const int total = selfTest.getTotal();
    const int passed = selfTest.getPassed();
    const bool failed = total > passed;
    const Color summaryColor = failed ? Color{ 200, 40, 40, 255 } : DARKGRAY;

    DrawText(TextFormat("SelfTest: %d/%d", passed, total), 1450, 10, 20, summaryColor);

    int y = 36;
    int shown = 0;
    for (const std::string& line : selfTest.getResults())
    {
        if (shown >= 20)
        {
            break;
        }
        const bool failed = line.rfind("FAIL", 0) == 0;
        DrawText(line.c_str(), 1450, y, 20, failed ? Color{ 200, 40, 40, 255 } : DARKGRAY);
        y += 26;
        ++shown;
    }
}

