#pragma once

#include "world/Tile.h"

namespace urbania {

// Individual city resident. Simulation data only: no movement,
// no rendering, no behavior yet. Future systems will assign
// workplaces, income changes, and happiness from these fields.
struct Citizen {
    int id = 0;
    TileCoordinate home{};
    TileCoordinate workplace{};
    float income = 0.0f;
    float happiness = 50.0f;
    bool employed = false;
};

}  // namespace urbania
