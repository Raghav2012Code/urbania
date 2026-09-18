#include "simulation/Population.h"

#include "world/Tile.h"
#include "world/World.h"

namespace {

constexpr float SIM_SECONDS_PER_HOUR = 3600.0f;

}  // namespace

void Population::update(World& world, float simulationDeltaTime)
{
    syncWithWorld(world);

    if (simulationDeltaTime > 0.0f)
    {
        for (auto& entry : homes)
        {
            ResidentialData& home = entry.second;
            home.growthProgress +=
                simulationDeltaTime / SIM_SECONDS_PER_HOUR * RESIDENTS_PER_SIM_HOUR;

            while (home.growthProgress >= 1.0f &&
                   countResidentsAt(entry.first.first, entry.first.second) < home.capacity)
            {
                home.growthProgress -= 1.0f;
                citizens.createCitizen(
                    { entry.first.first, entry.first.second, true });
            }

            // A full home holds no pending growth.
            if (countResidentsAt(entry.first.first, entry.first.second) >= home.capacity)
            {
                home.growthProgress = 0.0f;
            }
        }
    }

    totalPopulation = citizens.getCitizenCount();
    totalHousingCapacity = 0;
    for (const auto& entry : homes)
    {
        totalHousingCapacity += entry.second.capacity;
    }
}

int Population::getTotalPopulation() const
{
    return totalPopulation;
}

int Population::getTotalHousingCapacity() const
{
    return totalHousingCapacity;
}

int Population::getResidentsAt(int x, int y) const
{
    if (homes.find({ x, y }) == homes.end())
    {
        return 0;
    }
    return countResidentsAt(x, y);
}

const urbania::CitizenManager& Population::getCitizens() const
{
    return citizens;
}

int Population::countResidentsAt(int x, int y) const
{
    int count = 0;
    for (const urbania::Citizen& citizen : citizens.getCitizens())
    {
        if (citizen.home.valid && citizen.home.x == x && citizen.home.y == y)
        {
            ++count;
        }
    }
    return count;
}

void Population::syncWithWorld(const World& world)
{
    // Drop homes (and their citizens) for tiles that are no longer
    // Residential. No dangling residents: removal is by home tile.
    for (auto it = homes.begin(); it != homes.end();)
    {
        const Tile& tile = world.getTile(it->first.first, it->first.second);
        if (tile.type != TileType::Residential)
        {
            citizens.removeCitizensAt(
                { it->first.first, it->first.second, true });
            it = homes.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Register new Residential tiles as empty homes.
    for (int y = 0; y < world.getHeight(); ++y)
    {
        for (int x = 0; x < world.getWidth(); ++x)
        {
            if (world.getTile(x, y).type == TileType::Residential &&
                homes.find({ x, y }) == homes.end())
            {
                homes[{ x, y }] = { RESIDENTS_PER_RESIDENTIAL_TILE, 0.0f };
            }
        }
    }
}
