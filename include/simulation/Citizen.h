#pragma once

#include <vector>

#include "world/Tile.h"

namespace urbania {

// Individual city resident. Simulation data only: no rendering here,
// no behavior beyond commuting yet. Movement state (currentTile,
// pathIndex, movementProgress) tracks progress along commutePath;
// rendering interpolates from that state separately.
struct Citizen {
    int id = 0;
    TileCoordinate home{};
    TileCoordinate workplace{};
    std::vector<TileCoordinate> commutePath{};
    TileCoordinate currentTile{};
    int pathIndex = 0;
    float movementProgress = 0.0f;
    float income = 0.0f;
    float happiness = 50.0f;
    bool employed = false;
    bool commutingToWork = false;
};

}  // namespace urbania
