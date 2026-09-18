#include "core/Camera.h"

#include <algorithm>

#include "raylib.h"
#include "raymath.h"
#include "world/World.h"

namespace {

constexpr float PAN_SPEED = 900.0f;
constexpr float ZOOM_STEP = 0.15f;
constexpr float MIN_ZOOM = 0.25f;
constexpr float MAX_ZOOM = 3.0f;
// Minimum strip (in world pixels) of the world that must stay visible,
// so the camera can never leave the map entirely.
constexpr float MIN_VISIBLE_STRIP = 240.0f;

float worldPixelWidth()
{
    return static_cast<float>(WORLD_WIDTH * TILE_SIZE);
}

float worldPixelHeight()
{
    return static_cast<float>(WORLD_HEIGHT * TILE_SIZE);
}

}  // namespace

namespace urbania {

Camera::Camera()
{
    camera.target = { worldPixelWidth() / 2.0f, worldPixelHeight() / 2.0f };
    // View anchor left for update(): it re-pins offset to the live
    // window size every frame before the first draw.
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
}

void Camera::update(float deltaTime)
{
    // Keep the view anchor at the center of the current window.
    camera.offset = { static_cast<float>(GetScreenWidth()) / 2.0f,
                      static_cast<float>(GetScreenHeight()) / 2.0f };

    // Frame-rate independent panning with WASD. Speed is scaled by zoom
    // so the camera feels consistent at every zoom level.
    Vector2 direction = { 0.0f, 0.0f };
    if (IsKeyDown(KEY_A))
    {
        direction.x -= 1.0f;
    }
    if (IsKeyDown(KEY_D))
    {
        direction.x += 1.0f;
    }
    if (IsKeyDown(KEY_W))
    {
        direction.y -= 1.0f;
    }
    if (IsKeyDown(KEY_S))
    {
        direction.y += 1.0f;
    }

    if (direction.x != 0.0f || direction.y != 0.0f)
    {
        direction = Vector2Normalize(direction);
        const float step = PAN_SPEED * deltaTime / camera.zoom;
        camera.target = Vector2Add(camera.target, Vector2Scale(direction, step));
    }

    // Smooth exponential zoom around the mouse cursor.
    const float wheel = GetMouseWheelMove();
    if (wheel != 0.0f)
    {
        const Vector2 mouse = GetMousePosition();
        const Vector2 before = GetScreenToWorld2D(mouse, camera);

        camera.zoom = std::clamp(camera.zoom + wheel * ZOOM_STEP * camera.zoom, MIN_ZOOM, MAX_ZOOM);

        const Vector2 after = GetScreenToWorld2D(mouse, camera);
        camera.target = Vector2Add(camera.target, Vector2Subtract(before, after));
    }

    clampToWorldBounds();
}

void Camera::begin()
{
    BeginMode2D(camera);
}

void Camera::end()
{
    EndMode2D();
}

Vector2 Camera::screenToWorld(Vector2 screenPosition) const
{
    return GetScreenToWorld2D(screenPosition, camera);
}

Vector2 Camera::worldToScreen(Vector2 worldPosition) const
{
    return GetWorldToScreen2D(worldPosition, camera);
}

void Camera::clampToWorldBounds()
{
    // Visible half-extents in world coordinates shrink as zoom grows,
    // so the clamp range automatically accounts for the current zoom.
    const float halfViewWidth = static_cast<float>(GetScreenWidth()) / (2.0f * camera.zoom);
    const float halfViewHeight = static_cast<float>(GetScreenHeight()) / (2.0f * camera.zoom);

    const float minX = -halfViewWidth + MIN_VISIBLE_STRIP;
    const float maxX = worldPixelWidth() + halfViewWidth - MIN_VISIBLE_STRIP;
    const float minY = -halfViewHeight + MIN_VISIBLE_STRIP;
    const float maxY = worldPixelHeight() + halfViewHeight - MIN_VISIBLE_STRIP;

    if (minX > maxX)
    {
        camera.target.x = worldPixelWidth() / 2.0f;
    }
    else
    {
        camera.target.x = std::clamp(camera.target.x, minX, maxX);
    }

    if (minY > maxY)
    {
        camera.target.y = worldPixelHeight() / 2.0f;
    }
    else
    {
        camera.target.y = std::clamp(camera.target.y, minY, maxY);
    }
}

}  // namespace urbania
