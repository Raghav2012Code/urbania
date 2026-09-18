#pragma once

#include "world/Tile.h"

namespace urbania {

// Represents an individual bus stop located on a road tile.
// IDs are deterministic and 1-based.
struct BusStop {
    int id = 0;
    TileCoordinate tile;
};

}  // namespace urbania
