#include "core/Game.h"

#include "raylib.h"
#include "world/Tile.h"

namespace {

constexpr Color GRASS_FILL = { 126, 217, 87, 255 };
constexpr Color GRID_LINE = { 0, 0, 0, 30 };

}  // namespace

bool Game::initialize()
{
    const int screenWidth = 1920;
    const int screenHeight = 1080;

    InitWindow(screenWidth, screenHeight, "Urbania");
    SetTargetFPS(60);

    return IsWindowReady();
}

void Game::update(float deltaTime)
{
    camera.update(deltaTime);
}

void Game::draw()
{
    ClearBackground(RAYWHITE);

    camera.begin();
    drawWorld();
    camera.end();
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
