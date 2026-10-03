#include "simulation/Population.h"

#include <algorithm>

#include "world/Tile.h"
#include "world/World.h"

namespace {

constexpr float SIM_SECONDS_PER_HOUR = 3600.0f;

}  // namespace

void Population::update(World& world, float simulationDeltaTime)
{
    syncWithWorld(world);

    gridWidth = world.getWidth();
    gridHeight = world.getHeight();

    // One pass over the citizens builds every tile's resident count, so the
    // growth loop below is O(homes + citizens) instead of O(homes * citizens).
    const std::size_t tileCount = static_cast<std::size_t>(gridWidth) *
                                  static_cast<std::size_t>(gridHeight);
    if (residentCounts.size() != tileCount)
    {
        residentCounts.assign(tileCount, 0);
    }
    else
    {
        std::fill(residentCounts.begin(), residentCounts.end(), 0);
    }
    for (const urbania::Citizen& citizen : citizens.getCitizens())
    {
        if (citizen.home.valid && citizen.home.x >= 0 && citizen.home.x < gridWidth &&
            citizen.home.y >= 0 && citizen.home.y < gridHeight)
        {
            ++residentCounts[static_cast<std::size_t>(citizen.home.y) * gridWidth +
                             citizen.home.x];
        }
    }

    if (simulationDeltaTime > 0.0f)
    {
        for (auto& entry : homes)
        {
            ResidentialData& home = entry.second;
            const int homeX = entry.first.first;
            const int homeY = entry.first.second;
            const std::size_t index = static_cast<std::size_t>(homeY) * gridWidth + homeX;

            home.growthProgress +=
                simulationDeltaTime / SIM_SECONDS_PER_HOUR * RESIDENTS_PER_SIM_HOUR;

            while (home.growthProgress >= 1.0f &&
                   residentCounts[index] < home.capacity)
            {
                home.growthProgress -= 1.0f;
                citizens.createCitizen({ homeX, homeY, true });
                ++residentCounts[index];
            }

            // A full home holds no pending growth.
            if (residentCounts[index] >= home.capacity)
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
    return citizens.getCitizenCount();
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
    if (x < 0 || y < 0 || x >= gridWidth || y >= gridHeight)
    {
        return 0;
    }
    const std::size_t index = static_cast<std::size_t>(y) * gridWidth + x;
    if (index >= residentCounts.size())
    {
        return 0;
    }
    return residentCounts[index];
}

float Population::getGrowthProgress(int x, int y) const
{
    const auto it = homes.find({ x, y });
    if (it == homes.end())
    {
        return 0.0f;
    }
    return it->second.growthProgress;
}

void Population::setGrowthProgress(int x, int y, float progress)
{
    const auto it = homes.find({ x, y });
    if (it == homes.end())
    {
        return;
    }
    it->second.growthProgress = progress < 0.0f ? 0.0f : (progress > 1.0f ? 1.0f : progress);
}

const urbania::CitizenManager& Population::getCitizens() const
{
    return citizens;
}

urbania::CitizenManager& Population::getCitizenManager()
{
    return citizens;
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
