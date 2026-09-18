#pragma once

#include "raylib.h"

namespace urbania {

class Camera {
public:
    Camera();

    void update(float deltaTime);
    void begin();
    void end();

    Vector2 screenToWorld(Vector2 screenPosition) const;
    Vector2 worldToScreen(Vector2 worldPosition) const;

private:
    void clampToWorldBounds();

    Camera2D camera{};
};

}  // namespace urbania
