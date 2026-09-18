#include "core/Game.h"

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
    drawHighlight();
    drawPathTest();
    camera.end();

    drawDebugText();
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
}

void Game::handleBuildInput()
{
    // Build-type selection also leaves demolition mode.
    if (IsKeyPressed(KEY_ONE))
    {
        selectedBuildType = TileType::Road;
        demolishMode = false;
    }
    else if (IsKeyPressed(KEY_TWO))
    {
        selectedBuildType = TileType::Residential;
        demolishMode = false;
    }
    else if (IsKeyPressed(KEY_THREE))
    {
        selectedBuildType = TileType::Commercial;
        demolishMode = false;
    }
    else if (IsKeyPressed(KEY_FOUR))
    {
        selectedBuildType = TileType::Industrial;
        demolishMode = false;
    }
    else if (IsKeyPressed(KEY_FIVE))
    {
        selectedBuildType = TileType::Park;
        demolishMode = false;
    }
    else if (IsKeyPressed(KEY_D))
    {
        demolishMode = !demolishMode;
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

    Tile& tile = world.getTile(hovered.x, hovered.y);
    const TileType oldType = tile.type;

    bool changed = false;
    if (demolishMode)
    {
        changed = economy.tryDemolish(tile);
    }
    else
    {
        changed = economy.tryBuild(tile, selectedBuildType);
    }

    // Refresh the road graph only when a tile changes to or from Road.
    // Any successful edit also refreshes the temporary path test.
    if (changed && (oldType == TileType::Road || tile.type == TileType::Road))
    {
        simulation.getRoadNetwork().rebuild(world);
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
            DrawRectangle(x * tileSize, y * tileSize, tileSize, tileSize, tileColor(tile.type));
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

    if (tile.type != TileType::Grass)
    {
        // Occupied: cannot build here.
        DrawRectangleLinesEx(rect, 2.0f, BLOCKED_BORDER);
    }
    else if (!economy.canAfford(selectedBuildType))
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

void Game::drawDebugText()
{
    const urbania::TileCoordinate hovered = input.hovered();

    if (hovered.valid)
    {
        DrawText(TextFormat("Tile: (%d, %d)", hovered.x, hovered.y), 10, 10, 20, DARKGRAY);
    }
    else
    {
        DrawText("Tile: Outside world", 10, 10, 20, DARKGRAY);
    }

    if (demolishMode)
    {
        DrawText("Mode: DEMOLISH (D to exit)", 10, 36, 20, { 200, 40, 40, 255 });
    }
    else
    {
        DrawText(TextFormat("Build: %s", tileTypeName(selectedBuildType)), 10, 36, 20, DARKGRAY);
        DrawText(TextFormat("Cost: %s", formatMoney(Economy::getCost(selectedBuildType)).c_str()), 10,
                 62, 20, DARKGRAY);
    }

    DrawText(TextFormat("Money: %s", formatMoney(economy.getMoney()).c_str()), 10, 88, 20, DARKGRAY);
    DrawText(TextFormat("Day %d - %02d:%02d", simulationClock.getDay(), simulationClock.getHour(),
                        simulationClock.getMinute()),
             10, 114, 20, DARKGRAY);

    if (simulationClock.isPaused())
    {
        DrawText("PAUSED", 10, 140, 20, { 200, 40, 40, 255 });
    }
    else
    {
        DrawText(TextFormat("Speed: %dx", static_cast<int>(simulationClock.getTimeScale())), 10, 140,
                 20, DARKGRAY);
    }

    DrawText(TextFormat("Simulated: %.1fs", simulation.getElapsedSimulationSeconds()), 10, 166, 20,
             DARKGRAY);
    DrawText(TextFormat("Population: %d", simulation.getPopulation().getTotalPopulation()), 10, 192,
             20, DARKGRAY);
    DrawText(TextFormat("Housing: %d / %d", simulation.getPopulation().getTotalPopulation(),
                        simulation.getPopulation().getTotalHousingCapacity()),
             10, 218, 20, DARKGRAY);
    DrawText(TextFormat("Jobs: %d / %d", simulation.getEmployment().getOccupiedJobs(),
                        simulation.getEmployment().getTotalJobs()),
             10, 244, 20, DARKGRAY);
    DrawText(TextFormat("Employment: %d / %d", simulation.getEmployment().getEmployedCitizens(),
                        simulation.getPopulation().getTotalPopulation()),
             10, 270, 20, DARKGRAY);
    DrawText(TextFormat("Unemployed: %d", simulation.getEmployment().getUnemployedCitizens()), 10,
             296, 20, DARKGRAY);
    DrawText(TextFormat("Road Nodes: %d", simulation.getRoadNetwork().getNodeCount()), 10, 322, 20,
             DARKGRAY);

    if (pathTest.empty())
    {
        DrawText("Path Test: No Path", 10, 348, 20, DARKGRAY);
    }
    else
    {
        DrawText(TextFormat("Path Test Length: %d", static_cast<int>(pathTest.size())), 10, 348,
                 20, DARKGRAY);
    }

    if (hovered.valid &&
        simulation.getRoadNetwork().isRoad(hovered.x, hovered.y))
    {
        DrawText(TextFormat("Road Neighbors: %d",
                            static_cast<int>(simulation.getRoadNetwork()
                                                 .getNeighbors(hovered)
                                                 .size())),
                 10, 374, 20, DARKGRAY);
        DrawText("Keys: 1-5 select, D demolish, Space pause, F1-F4 speed", 10, 400, 20, DARKGRAY);
    }
    else
    {
        DrawText("Keys: 1-5 select, D demolish, Space pause, F1-F4 speed", 10, 374, 20, DARKGRAY);
    }
}
