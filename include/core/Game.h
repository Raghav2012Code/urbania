#pragma once

#include "core/Camera.h"
#include "core/Input.h"
#include "world/World.h"

class Game {
public:
    bool initialize();
    void update(float deltaTime);
    void draw();
    void shutdown();

private:
    void drawWorld();
    void drawHighlight();
    void drawDebugText();

    World world;
    urbania::Camera camera;
    urbania::Input input;
};
