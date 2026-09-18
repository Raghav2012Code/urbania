#pragma once

#include <map>
#include <utility>

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
    const urbania::CitizenManager& getCitizens() const;

private:
    void syncWithWorld(const World& world);
    int countResidentsAt(int x, int y) const;

    std::map<std::pair<int, int>, ResidentialData> homes;
    urbania::CitizenManager citizens;
    int totalPopulation = 0;
    int totalHousingCapacity = 0;
};
