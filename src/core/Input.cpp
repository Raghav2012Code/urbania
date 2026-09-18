#include "core/Input.h"

#include <cmath>

#include "raylib.h"
#include "world/World.h"

namespace urbania {

void Input::update(const Camera& camera)
{
    // Record left mouse clicks. No action is taken here; the flag is
    // exposed for future features (e.g. building).
    leftClickedThisFrame = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    // Mouse screen position -> world-space position through the camera,
    // so selection stays accurate while panning and zooming.
    const Vector2 worldPos = camera.screenToWorld(GetMousePosition());

    const int tileX = static_cast<int>(std::floor(worldPos.x / TILE_SIZE));
    const int tileY = static_cast<int>(std::floor(worldPos.y / TILE_SIZE));

    if (tileX >= 0 && tileX < WORLD_WIDTH && tileY >= 0 && tileY < WORLD_HEIGHT)
    {
        hoveredTile = { tileX, tileY, true };
    }
    else
    {
        hoveredTile = { -1, -1, false };
    }
}

TileCoordinate Input::hovered() const
{
    return hoveredTile;
}

bool Input::leftClicked() const
{
    return leftClickedThisFrame;
}

}  // namespace urbania
