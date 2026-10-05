#include "world/World.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

World::World()
    : tiles(WORLD_WIDTH * WORLD_HEIGHT)
{
    for (Tile& tile : tiles)
    {
        tile.type = TileType::Grass;
    }
}

int World::getWidth() const
{
    return WORLD_WIDTH;
}

int World::getHeight() const
{
    return WORLD_HEIGHT;
}

int World::getTileSize() const
{
    return TILE_SIZE;
}

Tile& World::getTile(int x, int y)
{
    if (!isValid(x, y))
    {
        throw std::out_of_range("World::getTile: coordinates (" + std::to_string(x) + ", " +
                                std::to_string(y) + ") out of bounds");
    }

    return tiles[y * WORLD_WIDTH + x];
}

const Tile& World::getTile(int x, int y) const
{
    if (!isValid(x, y))
    {
        throw std::out_of_range("World::getTile: coordinates (" + std::to_string(x) + ", " +
                                std::to_string(y) + ") out of bounds");
    }

    return tiles[y * WORLD_WIDTH + x];
}

bool World::isValid(int x, int y) const
{
    return x >= 0 && x < WORLD_WIDTH && y >= 0 && y < WORLD_HEIGHT;
}

bool World::hasParkNearby(int x, int y, int radius) const
{
    const int minX = std::max(0, x - radius);
    const int maxX = std::min(WORLD_WIDTH - 1, x + radius);
    const int minY = std::max(0, y - radius);
    const int maxY = std::min(WORLD_HEIGHT - 1, y + radius);
    for (int yy = minY; yy <= maxY; ++yy)
    {
        for (int xx = minX; xx <= maxX; ++xx)
        {
            if (tiles[yy * WORLD_WIDTH + xx].type == TileType::Park)
            {
                return true;
            }
        }
    }
    return false;
}

std::vector<Tile> World::snapshotTiles() const
{
    return tiles;
}

void World::restoreTiles(std::vector<Tile> tiles_) noexcept
{
    tiles = std::move(tiles_);
}
