#pragma once

#include <vector>

#include "world/Tile.h"

namespace urbania {

class CitizenManager;

}  // namespace urbania

class World;

namespace urbania {

// One workplace slot on a Commercial or Industrial tile.
struct Job {
    int id = 0;
    TileCoordinate workplace{};
    bool occupied = false;
};

// Matches unemployed citizens to jobs from Commercial and Industrial
// tiles. Jobs are derived from the World (never stored on tiles);
// matching is first-unemployed to first-vacant, fully deterministic.
// No distance, salary, or preference logic yet. No raylib dependency.
class Employment {
public:
    static constexpr int JOBS_PER_COMMERCIAL_TILE = 8;
    static constexpr int JOBS_PER_INDUSTRIAL_TILE = 15;

    void update(World& world, CitizenManager& citizens, float simulationDeltaTime);

    int getTotalJobs() const;
    int getOccupiedJobs() const;
    int getEmployedCitizens() const;
    int getUnemployedCitizens() const;
    const std::vector<Job>& getJobs() const;

private:
    static int jobsForTileType(TileType type);
    void syncJobsWithWorld(const World& world);
    void reconcileOccupancy(CitizenManager& citizens);
    void matchUnemployed(CitizenManager& citizens);

    std::vector<Job> jobs;
    int nextJobId = 1;
    int employedCitizens = 0;
    int unemployedCitizens = 0;
};

}  // namespace urbania
