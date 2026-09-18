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
