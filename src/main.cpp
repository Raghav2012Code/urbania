#include "core/Game.h"

#include "raylib.h"

int main()
{
    Game game;

    if (!game.initialize())
    {
        return 1;
    }

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();

        game.update(deltaTime);

        BeginDrawing();
        game.draw();
        EndDrawing();
    }

    game.shutdown();

    return 0;
}
