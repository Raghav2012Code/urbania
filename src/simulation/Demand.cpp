#include "simulation/Demand.h"

#include <algorithm>
#include <cmath>

#include "simulation/Employment.h"
#include "simulation/Population.h"
#include "world/World.h"

Demand::Demand()
{
}

int Demand::getResidentialDemand() const
{
    return residentialDemand;
}

int Demand::getCommercialDemand() const
{
    return commercialDemand;
}

int Demand::getIndustrialDemand() const
{
    return industrialDemand;
}

int Demand::getDemand(DemandType type) const
{
    switch (type)
    {
        case DemandType::Residential:
            return residentialDemand;
        case DemandType::Commercial:
            return commercialDemand;
        case DemandType::Industrial:
            return industrialDemand;
    }
    return 0;
}

void Demand::update(const World& world, const Population& population,
                    const urbania::Employment& employment, float simulationDeltaTime)
{
    // Zero or negative delta (e.g. paused) halts simulation progression.
    if (simulationDeltaTime <= 0.0f)
    {
        return;
    }

    secondsTowardNextHour += simulationDeltaTime;
    while (secondsTowardNextHour >= SIM_SECONDS_PER_HOUR)
    {
        secondsTowardNextHour -= SIM_SECONDS_PER_HOUR;
        recalculate(world, population, employment);
    }
}

void Demand::recalculate(const World& world, const Population& population,
                         const urbania::Employment& employment)
{
    const int totalPop = population.getTotalPopulation();
    const int housingCap = population.getTotalHousingCapacity();

    // Count total and occupied jobs per commercial/industrial sector.
    int totalCommercialJobs = 0;
    int occupiedCommercialJobs = 0;
    int totalIndustrialJobs = 0;
    int occupiedIndustrialJobs = 0;

    for (const urbania::Job& job : employment.getJobs())
    {
        if (!job.workplace.valid || job.workplace.x < 0 || job.workplace.y < 0 ||
            job.workplace.x >= world.getWidth() || job.workplace.y >= world.getHeight())
        {
            continue;
        }

        const TileType type = world.getTile(job.workplace.x, job.workplace.y).type;
        if (type == TileType::Commercial)
        {
            ++totalCommercialJobs;
            if (job.occupied)
            {
                ++occupiedCommercialJobs;
            }
        }
        else if (type == TileType::Industrial)
        {
            ++totalIndustrialJobs;
            if (job.occupied)
            {
                ++occupiedIndustrialJobs;
            }
        }
    }

    residentialDemand = calculateResidentialDemand(totalPop, housingCap);
    commercialDemand =
        calculateCommercialDemand(totalPop, totalCommercialJobs, occupiedCommercialJobs);
    industrialDemand =
        calculateIndustrialDemand(totalPop, totalIndustrialJobs, occupiedIndustrialJobs);
}

int Demand::calculateResidentialDemand(int population, int housingCapacity) const
{
    // If no housing capacity exists:
    // - If population is also 0 (empty city), demand is neutral (0).
    // - If population somehow exceeds 0 with 0 capacity, demand is max (+100).
    if (housingCapacity <= 0)
    {
        return (population > 0) ? MAX_DEMAND : 0;
    }

    // Housing occupancy ratio: 0.0 (all empty) to 1.0 (full capacity).
    // - Occupancy 0.0 (large surplus / empty homes) yields -100 demand.
    // - Occupancy 0.5 (half full) yields 0 neutral demand.
    // - Occupancy 1.0 (approaching/at full capacity) yields +100 demand.
    const float occupancy =
        static_cast<float>(population) / static_cast<float>(housingCapacity);
    const float score = (occupancy - 0.5f) * 200.0f;
    return std::clamp(static_cast<int>(std::round(score)), MIN_DEMAND, MAX_DEMAND);
}

int Demand::calculateCommercialDemand(int population, int totalJobs, int occupiedJobs) const
{
    // If no commercial jobs exist:
    // - If population is 0, demand is neutral (0).
    // - If citizens exist with no commercial services, demand grows with population.
    if (totalJobs <= 0)
    {
        return (population > 0) ? std::clamp(population * 10, 0, MAX_DEMAND) : 0;
    }

    // Commercial demand balances two deterministic forces:
    // 1. Occupancy factor [-50, +50]:
    //    Fully occupied commercial slots (+50) indicate thriving shops needing expansion;
    //    many vacant commercial slots (-50) indicate excess commercial capacity.
    const float occupancy =
        static_cast<float>(occupiedJobs) / static_cast<float>(totalJobs);
    const float occupancyScore = (occupancy - 0.5f) * 100.0f;

    // 2. Population service target [-50, +50]:
    //    Target ratio is ~1 commercial job per 2 citizens (0.5x population).
    //    A shortage of commercial jobs increases demand; a surplus reduces it.
    const float targetJobs = static_cast<float>(population) * 0.5f;
    const float shortage = targetJobs - static_cast<float>(totalJobs);
    const float shortageScore = std::clamp(shortage * 10.0f, -50.0f, 50.0f);

    const float totalScore = occupancyScore + shortageScore;
    return std::clamp(static_cast<int>(std::round(totalScore)), MIN_DEMAND, MAX_DEMAND);
}

int Demand::calculateIndustrialDemand(int population, int totalJobs, int occupiedJobs) const
{
    // If no industrial jobs exist:
    // - If population is 0, demand is neutral (0).
    // - If citizens exist with no industrial employment, demand grows with population.
    if (totalJobs <= 0)
    {
        return (population > 0) ? std::clamp(population * 10, 0, MAX_DEMAND) : 0;
    }

    // Industrial demand balances two deterministic forces:
    // 1. Occupancy factor [-50, +50]:
    //    Fully staffed industrial slots (+50) indicate full factories needing expansion;
    //    vacant industrial slots (-50) indicate excess industrial capacity.
    const float occupancy =
        static_cast<float>(occupiedJobs) / static_cast<float>(totalJobs);
    const float occupancyScore = (occupancy - 0.5f) * 100.0f;

    // 2. Workforce target [-50, +50]:
    //    Target ratio is ~1 industrial job per 2 citizens (0.5x population).
    //    A shortage of industrial jobs increases demand; a surplus reduces it.
    const float targetJobs = static_cast<float>(population) * 0.5f;
    const float shortage = targetJobs - static_cast<float>(totalJobs);
    const float shortageScore = std::clamp(shortage * 10.0f, -50.0f, 50.0f);

    const float totalScore = occupancyScore + shortageScore;
    return std::clamp(static_cast<int>(std::round(totalScore)), MIN_DEMAND, MAX_DEMAND);
}
