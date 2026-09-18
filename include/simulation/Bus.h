#pragma once

#include <vector>

#include "world/Tile.h"

namespace urbania {

class RoadNetwork;
class Transit;

// Bus vehicle that travels along an ordered BusRoute.
// Follows an A* road path from stop to stop in sequence and loops indefinitely.
// Simulation data and deterministic movement only; independent from raylib rendering.
struct Bus {
    static constexpr float BUS_SPEED_TILES_PER_SECOND = 3.0f;
    static constexpr float DEFAULT_SPEED = BUS_SPEED_TILES_PER_SECOND;

    int id = 0;
    int routeId = 0;

    std::vector<TileCoordinate> path{};
    int pathIndex = 0;
    float movementProgress = 0.0f;

    int currentStopIndex = 0;

    float speed = DEFAULT_SPEED;
    bool active = false;

    // Movement update along the current stop-to-stop path.
    // When the path completes, requests the next leg via transit and roadNetwork.
    void update(float deltaTime, const Transit& transit, const RoadNetwork& roadNetwork);

    // Rebuilds path to next stop in route
    bool buildNextLeg(const Transit& transit, const RoadNetwork& roadNetwork);
};

}  // namespace urbania
