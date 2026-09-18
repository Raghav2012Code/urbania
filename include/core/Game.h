#pragma once

#include "core/Camera.h"
#include "world/World.h"

class Game {
public:
    bool initialize();
    void update(float deltaTime);
    void draw();
    void shutdown();

private:
    void drawWorld();

    World world;
    urbania::Camera camera;
};
