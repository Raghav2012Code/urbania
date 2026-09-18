#include "world/World.h"

#include <stdexcept>
#include <string>

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
