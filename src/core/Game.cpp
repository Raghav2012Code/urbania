#include "core/Game.h"

#include "raylib.h"

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
    (void)deltaTime;
}

void Game::draw()
{
    ClearBackground(RAYWHITE);
}

void Game::shutdown()
{
    CloseWindow();
}
