#include "rendering/EntityRenderer.h"

#include <algorithm>
#include <cmath>

#include "raylib.h"
#include "rlgl.h"
#include "simulation/CitizenManager.h"
#include "simulation/Traffic.h"
#include "simulation/Transit.h"

namespace urbania {

EntityRenderer::EntityRenderer()
{
}

void EntityRenderer::renderCitizens(const CitizenManager& citizens, int tileSize)
{
    const float halfTile = static_cast<float>(tileSize) / 2.0f;

    for (const Citizen& citizen : citizens.getCitizens())
    {
        if (!citizen.currentTile.valid)
        {
            continue;
        }

        TileCoordinate target = citizen.currentTile;
        if (citizen.pathIndex >= 0 &&
            citizen.pathIndex < static_cast<int>(citizen.commutePath.size()))
        {
            target = citizen.commutePath[citizen.pathIndex];
        }

        const float x = (static_cast<float>(citizen.currentTile.x) +
                         (static_cast<float>(target.x - citizen.currentTile.x)) *
                             citizen.movementProgress) *
                            static_cast<float>(tileSize) +
                        halfTile;
        const float y = (static_cast<float>(citizen.currentTile.y) +
                         (static_cast<float>(target.y - citizen.currentTile.y)) *
                             citizen.movementProgress) *
                            static_cast<float>(tileSize) +
                        halfTile;

        drawPedestrian(x, y, citizen.id, citizen.movementProgress);
    }
}

void EntityRenderer::drawPedestrian(float x, float y, int citizenId, float progress)
{
    const Color coatColors[] = {
        Color{ 45, 120, 230, 255 },  // Blue
        Color{ 220, 60, 50, 255 },   // Red
        Color{ 40, 160, 80, 255 },   // Green
        Color{ 230, 160, 30, 255 },  // Amber
        Color{ 160, 60, 200, 255 },  // Purple
        Color{ 60, 70, 80, 255 }     // Slate
    };
    const Color coat = coatColors[static_cast<size_t>(std::abs(citizenId)) % 6];

    // Subtle walking swing
    const float stride = std::sin(progress * 3.14159f * 4.0f) * 1.5f;

    // Drop shadow
    DrawCircle(static_cast<int>(x) + 1, static_cast<int>(y) + 1, 3.5f, Color{ 0, 0, 0, 80 });

    // Shoulders / Torso
    DrawCircle(static_cast<int>(x), static_cast<int>(y), 3.0f, coat);

    // Head / Hair
    DrawCircle(static_cast<int>(x), static_cast<int>(y) - 1, 2.0f, Color{ 235, 195, 150, 255 });
    DrawCircle(static_cast<int>(x), static_cast<int>(y) - 2, 1.5f, Color{ 60, 40, 30, 255 }); // hair

    // Feet dots
    DrawCircle(static_cast<int>(x + stride), static_cast<int>(y) + 2, 1.0f, Color{ 30, 30, 30, 255 });
}

void EntityRenderer::renderVehicles(const Traffic& traffic, int tileSize)
{
    const float halfTile = static_cast<float>(tileSize) / 2.0f;

    for (const Vehicle& vehicle : traffic.getVehicles())
    {
        if (!vehicle.active || vehicle.path.empty())
        {
            continue;
        }

        const int last = static_cast<int>(vehicle.path.size()) - 1;
        const int fromIndex = std::clamp(vehicle.pathIndex, 0, last);
        const int toIndex = std::min(fromIndex + 1, last);
        const TileCoordinate& from = vehicle.path[fromIndex];
        const TileCoordinate& to = vehicle.path[toIndex];

        const float x = (static_cast<float>(from.x) +
                         (static_cast<float>(to.x - from.x)) * vehicle.movementProgress) *
                            static_cast<float>(tileSize) +
                        halfTile;
        const float y = (static_cast<float>(from.y) +
                         (static_cast<float>(to.y - from.y)) * vehicle.movementProgress) *
                            static_cast<float>(tileSize) +
                        halfTile;

        const float heading = std::atan2(static_cast<float>(to.y - from.y),
                                         static_cast<float>(to.x - from.x));

        drawCar(x, y, heading, vehicle.id);
    }
}

void EntityRenderer::drawCar(float x, float y, float headingRad, int vehicleId)
{
    const Color carColors[] = {
        Color{ 220, 50, 45, 255 },   // Crimson
        Color{ 45, 110, 210, 255 },  // Royal Blue
        Color{ 240, 240, 245, 255 }, // White
        Color{ 245, 195, 35, 255 },  // Yellow / Taxi
        Color{ 45, 160, 75, 255 },   // Green
        Color{ 40, 45, 55, 255 }     // Midnight Black
    };
    const Color bodyColor = carColors[static_cast<size_t>(std::abs(vehicleId)) % 6];

    const float deg = headingRad * (180.0f / 3.14159265f);

    rlPushMatrix();
    rlTranslatef(x, y, 0.0f);
    rlRotatef(deg, 0.0f, 0.0f, 1.0f);

    // Drop shadow
    DrawRectangleRounded(Rectangle{ -6.0f, -3.0f, 12.0f, 8.0f }, 0.4f, 4, Color{ 0, 0, 0, 80 });

    // Car Body (length 11, width 6)
    DrawRectangleRounded(Rectangle{ -5.5f, -3.5f, 11.0f, 7.0f }, 0.4f, 4, bodyColor);
    DrawRectangleRoundedLines(Rectangle{ -5.5f, -3.5f, 11.0f, 7.0f }, 0.4f, 4, Color{ 20, 25, 30, 200 });

    // Windshield (front) & Rear Window
    DrawRectangleRec(Rectangle{ 0.5f, -2.5f, 2.5f, 5.0f }, Color{ 120, 200, 240, 255 });
    DrawRectangleRec(Rectangle{ -4.0f, -2.5f, 1.8f, 5.0f }, Color{ 80, 140, 180, 255 });

    // Headlights (front right)
    DrawCircle(5, -2, 1.0f, Color{ 255, 255, 200, 255 });
    DrawCircle(5, 2, 1.0f, Color{ 255, 255, 200, 255 });

    // Taillights (rear left)
    DrawCircle(-5, -2, 1.0f, Color{ 240, 30, 30, 255 });
    DrawCircle(-5, 2, 1.0f, Color{ 240, 30, 30, 255 });

    rlPopMatrix();
}

void EntityRenderer::renderBuses(const Transit& transit, int tileSize)
{
    const float halfTile = static_cast<float>(tileSize) / 2.0f;

    for (const Bus& bus : transit.getBuses())
    {
        if (!bus.active || bus.path.empty())
        {
            continue;
        }

        const int last = static_cast<int>(bus.path.size()) - 1;
        const int fromIndex = std::clamp(bus.pathIndex, 0, last);
        const int toIndex = std::min(fromIndex + 1, last);
        const TileCoordinate& from = bus.path[fromIndex];
        const TileCoordinate& to = bus.path[toIndex];

        const float x = (static_cast<float>(from.x) +
                         (static_cast<float>(to.x - from.x)) * bus.movementProgress) *
                            static_cast<float>(tileSize) +
                        halfTile;
        const float y = (static_cast<float>(from.y) +
                         (static_cast<float>(to.y - from.y)) * bus.movementProgress) *
                            static_cast<float>(tileSize) +
                        halfTile;

        const float heading = std::atan2(static_cast<float>(to.y - from.y),
                                         static_cast<float>(to.x - from.x));

        drawBus(x, y, heading, bus.id, bus.routeId);
    }
}

void EntityRenderer::drawBus(float x, float y, float headingRad, int busId, int routeId)
{
    (void)busId;
    const float deg = headingRad * (180.0f / 3.14159265f);

    const Color routeColors[] = {
        Color{ 180, 50, 220, 255 },  // Purple
        Color{ 40, 160, 220, 255 },  // Cyan
        Color{ 230, 120, 30, 255 },  // Orange
        Color{ 50, 200, 100, 255 },  // Green
        Color{ 220, 60, 100, 255 }   // Pink/Red
    };
    const Color stripeColor = routeColors[static_cast<size_t>(std::abs(routeId)) % 5];

    rlPushMatrix();
    rlTranslatef(x, y, 0.0f);
    rlRotatef(deg, 0.0f, 0.0f, 1.0f);

    // Drop shadow
    DrawRectangleRounded(Rectangle{ -9.0f, -4.5f, 18.0f, 10.0f }, 0.3f, 4, Color{ 0, 0, 0, 90 });

    // Elongated Bus Body (length 17, width 9)
    DrawRectangleRounded(Rectangle{ -8.5f, -4.5f, 17.0f, 9.0f }, 0.3f, 4, Color{ 255, 215, 0, 255 }); // Gold transit yellow
    DrawRectangleRoundedLines(Rectangle{ -8.5f, -4.5f, 17.0f, 9.0f }, 0.3f, 4, Color{ 25, 40, 70, 220 });

    // Route stripe band across the center roof
    DrawRectangleRec(Rectangle{ -4.0f, -4.5f, 8.0f, 2.0f }, stripeColor);
    DrawRectangleRec(Rectangle{ -4.0f, 2.5f, 8.0f, 2.0f }, stripeColor);

    // Large front windshield
    DrawRectangleRec(Rectangle{ 3.5f, -3.5f, 3.5f, 7.0f }, Color{ 100, 190, 235, 255 });

    // Passenger side window rows
    DrawRectangleRec(Rectangle{ -6.5f, -3.8f, 8.5f, 1.5f }, Color{ 60, 120, 160, 255 });
    DrawRectangleRec(Rectangle{ -6.5f, 2.3f, 8.5f, 1.5f }, Color{ 60, 120, 160, 255 });

    // Headlights
    DrawCircle(8, -3, 1.2f, Color{ 255, 255, 220, 255 });
    DrawCircle(8, 3, 1.2f, Color{ 255, 255, 220, 255 });

    // Brake lights
    DrawCircle(-8, -3, 1.2f, Color{ 240, 40, 40, 255 });
    DrawCircle(-8, 3, 1.2f, Color{ 240, 40, 40, 255 });

    rlPopMatrix();
}

}  // namespace urbania
