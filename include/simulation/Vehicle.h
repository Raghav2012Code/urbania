#pragma once

#include <vector>

#include "world/Tile.h"

namespace urbania {

// One commute trip by road. Follows a copy of its citizen's commute
// route (road tiles only); pathIndex is the current leg exactly like
// Citizen movement, so rendering interpolates the same way.
// Simulation data only: no physics, no rendering here.
struct Vehicle {
    int id = 0;
    int citizenId = 0;
    std::vector<TileCoordinate> path{};
    int pathIndex = 0;
    float movementProgress = 0.0f;
    float speed = 4.0f;
    bool active = false;
};

}  // namespace urbania
