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

// Shared simulation-hour constant. One source of truth; all hourly
// accumulators (Demand/Housing/Happiness/LandValue/Pollution/Population)
// advance on this.
inline constexpr float kSimSecondsPerHour = 3600.0f;

// Tile position shared by input, simulation, and citizens.
// An invalid coordinate (valid == false) means "no tile assigned".
struct TileCoordinate {
    int x = -1;
    int y = -1;
    bool valid = false;

    static TileCoordinate validCoord(int x_, int y_)
    {
        TileCoordinate c;
        c.x = x_;
        c.y = y_;
        c.valid = true;
        return c;
    }

    // NOTE: `valid` participates in ordering so grid maps keyed by
    // valid coordinates never collide with invalid lookups. Always use
    // validCoord(x, y) (or {x, y, true}) for grid keys and early-out on
    // !coord.valid before lookups.
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
