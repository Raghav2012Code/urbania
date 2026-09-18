#pragma once

#include <vector>

#include "world/Tile.h"

constexpr int WORLD_WIDTH = 80;
constexpr int WORLD_HEIGHT = 80;
constexpr int TILE_SIZE = 32;

class World {
public:
    World();

    int getWidth() const;
    int getHeight() const;
    int getTileSize() const;

    Tile& getTile(int x, int y);
    const Tile& getTile(int x, int y) const;

private:
    bool isValid(int x, int y) const;

    std::vector<Tile> tiles;
};
