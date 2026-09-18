#pragma once

#include "world/Tile.h"

// Centralized construction costs and player money. Future economy
// features build on this same component.
class Economy {
public:
    static constexpr int STARTING_MONEY = 100000;
    static constexpr int ROAD_COST = 100;
    static constexpr int RESIDENTIAL_COST = 2000;
    static constexpr int COMMERCIAL_COST = 5000;
    static constexpr int INDUSTRIAL_COST = 10000;
    static constexpr int PARK_COST = 1000;

    Economy();

    int getMoney() const;

    static int getCost(TileType type);
    static bool isBuildable(TileType type);
    static bool isDeveloped(const Tile& tile);

    bool canAfford(TileType type) const;
    bool tryBuild(Tile& tile, TileType type);
    bool tryDemolish(Tile& tile);

private:
    int money;
};
