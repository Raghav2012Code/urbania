#include "simulation/Economy.h"

Economy::Economy()
    : money(STARTING_MONEY)
{
}

int Economy::getMoney() const
{
    return money;
}

int Economy::getCost(TileType type)
{
    switch (type)
    {
        case TileType::Road:
            return ROAD_COST;
        case TileType::Residential:
            return RESIDENTIAL_COST;
        case TileType::Commercial:
            return COMMERCIAL_COST;
        case TileType::Industrial:
            return INDUSTRIAL_COST;
        case TileType::Park:
            return PARK_COST;
        case TileType::Grass:
            return 0;
    }

    return 0;
}

bool Economy::isBuildable(TileType type)
{
    return getCost(type) > 0;
}

bool Economy::isDeveloped(const Tile& tile)
{
    return tile.type != TileType::Grass;
}

bool Economy::canAfford(TileType type) const
{
    return money >= getCost(type);
}

bool Economy::tryBuild(Tile& tile, TileType type)
{
    // Only buildable types, only on free Grass, only when affordable.
    if (!isBuildable(type))
    {
        return false;
    }
    if (tile.type != TileType::Grass)
    {
        return false;
    }
    if (money < getCost(type))
    {
        return false;
    }

    money -= getCost(type);
    tile.type = type;
    return true;
}

bool Economy::tryDemolish(Tile& tile)
{
    // Only developed tiles can be demolished. No refund.
    if (tile.type == TileType::Grass)
    {
        return false;
    }

    tile.type = TileType::Grass;
    return true;
}
