#pragma once

#include <vector>

#include "world/Tile.h"

namespace urbania {

// One commuter vehicle. Follows a copy of its citizen's commute route
// (road tiles only); pathIndex is the current leg exactly like Citizen
// movement, so rendering interpolates the same way.
//
// The vehicle shuttles: on reaching one end of the route it reverses
// direction and drives back, so a single Vehicle object serves a commuter
// for the whole session instead of being destroyed and reallocated at
// every trip. Simulation data only: no physics, no rendering here.
struct Vehicle {
    int id = 0;
    int citizenId = 0;
    std::vector<TileCoordinate> path{};
    int pathIndex = 0;
    float movementProgress = 0.0f;
    float speed = 4.0f;
    bool active = false;

    // +1 travels toward the workplace end of the route, -1 back toward
    // home. A vehicle shuttles along its route indefinitely rather than
    // being retired and reallocated at each end, which keeps vehicle IDs
    // stable for rendering and keeps traffic on the roads continuously.
    int direction = 1;
};

}  // namespace urbania
