#include "core/Game.h"

#include "raylib.h"
#include "world/Tile.h"

namespace {

constexpr Color GRASS_FILL = { 126, 217, 87, 255 };
constexpr Color GRID_LINE = { 0, 0, 0, 30 };
constexpr Color HIGHLIGHT_FILL = { 255, 255, 255, 80 };
constexpr Color HIGHLIGHT_BORDER = { 255, 203, 5, 255 };

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

void Game::drawWorld()
{
    const int tileSize = world.getTileSize();

    for (int y = 0; y < world.getHeight(); ++y)
    {
        for (int x = 0; x < world.getWidth(); ++x)
        {
            const Tile& tile = world.getTile(x, y);

            Color color = GRASS_FILL;
            if (tile.type != TileType::Grass)
            {
                color = RAYWHITE;
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

    DrawRectangle(px, py, tileSize, tileSize, HIGHLIGHT_FILL);
    DrawRectangleLinesEx({ static_cast<float>(px), static_cast<float>(py),
                           static_cast<float>(tileSize), static_cast<float>(tileSize) },
                         2.0f, HIGHLIGHT_BORDER);
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
}
