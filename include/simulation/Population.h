#pragma once

#include <map>
#include <utility>
#include <vector>

#include "simulation/CitizenManager.h"

class World;

// Tracks housing capacity and residents for Residential tiles.
// Simulation state lives here, separate from basic tile identity:
// Tile only says *what* a tile is, Population says *who lives there*.
//
// Growth creates real Citizen entities (one per resident) through the
// CitizenManager, so totalPopulation always equals the active citizen
// count. Demolishing a home removes its citizens with it.
//
// Deterministic: growth is a pure function of accumulated simulation
// time, so the same world state + sim time always yields the same
// population. No raylib dependency here.
struct ResidentialData {
    int capacity = 0;
    float growthProgress = 0.0f;
};

class Population {
public:
    static constexpr int RESIDENTS_PER_RESIDENTIAL_TILE = 10;
    static constexpr float RESIDENTS_PER_SIM_HOUR = 1.0f;

    void update(World& world, float simulationDeltaTime);

    int getTotalPopulation() const;
    int getTotalHousingCapacity() const;
    int getResidentsAt(int x, int y) const;
    float getGrowthProgress(int x, int y) const;
    void setGrowthProgress(int x, int y, float progress);
    const urbania::CitizenManager& getCitizens() const;
    urbania::CitizenManager& getCitizenManager();

private:
    void syncWithWorld(const World& world);

    std::map<std::pair<int, int>, ResidentialData> homes;
    urbania::CitizenManager citizens;

    // Flat per-tile resident counts, indexed y * gridWidth + x. Rebuilt in a
    // single pass over the citizen list at the top of update(), so growth and
    // getResidentsAt() are O(1) per home instead of O(citizens) each.
    std::vector<int> residentCounts;
    int gridWidth = 0;
    int gridHeight = 0;

    int totalPopulation = 0;
    int totalHousingCapacity = 0;
};
