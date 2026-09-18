#pragma once

enum class TileType {
    Grass
};

struct Tile {
    TileType type = TileType::Grass;
};
