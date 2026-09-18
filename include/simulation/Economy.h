#pragma once

#include "world/Tile.h"

class World;
class Population;

namespace urbania {
class Employment;
}

// City financial state: construction wallet plus a simple deterministic
// daily economy (citizen/workplace taxes minus per-tile maintenance).
//
// Construction spending is immediate and never drives money negative.
// Taxes and maintenance settle once per simulation day from accumulated
// simulation time (never per rendered frame), so pause halts the economy
// and faster simulation speeds only make the daily settlement arrive
// sooner in real time.
class Economy {
public:
    static constexpr int STARTING_MONEY = 100000;
    static constexpr int ROAD_COST = 100;
    static constexpr int RESIDENTIAL_COST = 2000;
    static constexpr int COMMERCIAL_COST = 5000;
    static constexpr int INDUSTRIAL_COST = 10000;
    static constexpr int PARK_COST = 1000;

    // Simulation-time income values (per simulation day).
    static constexpr float RESIDENTIAL_TAX_PER_CITIZEN = 100.0f;
    static constexpr float COMMERCIAL_TAX_PER_JOB = 50.0f;
    static constexpr float INDUSTRIAL_TAX_PER_JOB = 75.0f;

    // Daily maintenance per existing tile.
    static constexpr float ROAD_MAINTENANCE = 2.0f;
    static constexpr float RESIDENTIAL_MAINTENANCE = 5.0f;
    static constexpr float COMMERCIAL_MAINTENANCE = 10.0f;
    static constexpr float INDUSTRIAL_MAINTENANCE = 15.0f;
    static constexpr float PARK_MAINTENANCE = 5.0f;

    static constexpr float SIM_SECONDS_PER_DAY = 86400.0f;

    Economy();

    int getMoney() const;

    // Generic wallet operations. spendMoney never drives money negative.
    bool canAfford(int amount) const;
    void addMoney(int amount);
    bool spendMoney(int amount);

    static int getCost(TileType type);
    static bool isBuildable(TileType type);
    static bool isDeveloped(const Tile& tile);
    static float getMaintenanceForTile(TileType type);

    bool canAfford(TileType type) const;
    bool tryBuild(Tile& tile, TileType type);
    bool tryDemolish(Tile& tile);

    // Advances the daily ledger on simulation time. Call once per frame
    // with the scaled simulation delta (0 while paused).
    void update(const World& world, const Population& population,
                const urbania::Employment& employment, float simulationDeltaTime);

    // Last settled day's figures (zero until the first full day passes).
    float getTaxIncome() const;
    float getMaintenanceCost() const;
    float getNetIncome() const;
    float getTotalTaxCollected() const;
    float getTotalMaintenancePaid() const;

private:
    void settleDay(const World& world, const Population& population,
                   const urbania::Employment& employment);

    int money;
    float taxIncome = 0.0f;
    float maintenanceCost = 0.0f;
    float netIncome = 0.0f;
    float totalTaxCollected = 0.0f;
    float totalMaintenancePaid = 0.0f;
    float secondsTowardNextDay = 0.0f;
};
