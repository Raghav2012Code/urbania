#include "core/Game.h"

#include <string>

#include "raylib.h"
#include "world/Tile.h"

namespace {

constexpr Color GRASS_FILL = { 126, 217, 87, 255 };
constexpr Color ROAD_FILL = { 105, 105, 105, 255 };
constexpr Color RESIDENTIAL_FILL = { 80, 150, 255, 255 };
constexpr Color COMMERCIAL_FILL = { 255, 170, 30, 255 };
constexpr Color INDUSTRIAL_FILL = { 190, 70, 70, 255 };
constexpr Color PARK_FILL = { 35, 140, 60, 255 };
constexpr Color GRID_LINE = { 0, 0, 0, 30 };
constexpr Color HIGHLIGHT_FILL = { 255, 255, 255, 80 };
constexpr Color HIGHLIGHT_BORDER = { 255, 203, 5, 255 };
constexpr Color PREVIEW_FILL = { 255, 255, 255, 110 };
constexpr Color BLOCKED_BORDER = { 220, 50, 50, 255 };
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

    return IsWindowReady();
}

void Game::update(float deltaTime)
{
    camera.update(deltaTime);
    input.update(camera);
    handleBuildInput();
}

void Game::draw()
{
    ClearBackground(RAYWHITE);

    camera.begin();
    drawWorld();
    drawHighlight();
    camera.end();

    drawDebugText();
}

void Game::shutdown()
{
    CloseWindow();
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
    if (demolishMode)
    {
        economy.tryDemolish(tile);
    }
    else
    {
        economy.tryBuild(tile, selectedBuildType);
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
            DrawRectangle(px, py, tileSize, tileSize, { 220, 50, 50, 110 });
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
    DrawText("Keys: 1-5 select, D demolish", 10, 114, 20, DARKGRAY);
}
