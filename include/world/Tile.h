#pragma once

enum class TileType {
    Grass,
    Road,
    Residential,
    Commercial,
    Industrial,
    Park
};

struct Tile {
    TileType type = TileType::Grass;
};

namespace urbania {

// Tile position shared by input, simulation, and citizens.
// An invalid coordinate (valid == false) means "no tile assigned".
struct TileCoordinate {
    int x = -1;
    int y = -1;
    bool valid = false;
};

}  // namespace urbania
