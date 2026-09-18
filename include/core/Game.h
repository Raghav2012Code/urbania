#pragma once

class Game {
public:
    bool initialize();
    void update(float deltaTime);
    void draw();
    void shutdown();
};
