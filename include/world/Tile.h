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

    bool operator<(const TileCoordinate& other) const
    {
        if (x != other.x)
        {
            return x < other.x;
        }
        if (y != other.y)
        {
            return y < other.y;
        }
        return valid < other.valid;
    }

    bool operator==(const TileCoordinate& other) const
    {
        return x == other.x && y == other.y && valid == other.valid;
    }

    bool operator!=(const TileCoordinate& other) const
    {
        return !(*this == other);
    }
};

}  // namespace urbania
