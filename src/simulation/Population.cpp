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

            while (home.growthProgress >= 1.0f && home.residents < home.capacity)
            {
                home.residents += 1;
                home.growthProgress -= 1.0f;
            }

            // A full home holds no pending growth.
            if (home.residents >= home.capacity)
            {
                home.growthProgress = 0.0f;
            }
        }
    }

    totalPopulation = 0;
    totalHousingCapacity = 0;
    for (const auto& entry : homes)
    {
        totalPopulation += entry.second.residents;
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
    const auto it = homes.find({ x, y });
    if (it == homes.end())
    {
        return 0;
    }
    return it->second.residents;
}

void Population::syncWithWorld(const World& world)
{
    // Drop data for tiles that are no longer Residential (demolished or
    // replaced); their residents leave with them.
    for (auto it = homes.begin(); it != homes.end();)
    {
        const Tile& tile = world.getTile(it->first.first, it->first.second);
        if (tile.type != TileType::Residential)
        {
            it = homes.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Register new Residential tiles with empty homes.
    for (int y = 0; y < world.getHeight(); ++y)
    {
        for (int x = 0; x < world.getWidth(); ++x)
        {
            if (world.getTile(x, y).type == TileType::Residential &&
                homes.find({ x, y }) == homes.end())
            {
                homes[{ x, y }] = { RESIDENTS_PER_RESIDENTIAL_TILE, 0, 0.0f };
            }
        }
    }
}
