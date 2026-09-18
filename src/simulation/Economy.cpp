#include "simulation/Economy.h"

#include <algorithm>

#include "simulation/Employment.h"
#include "simulation/Population.h"
#include "world/World.h"

Economy::Economy()
    : money(STARTING_MONEY)
{
}

int Economy::getMoney() const
{
    return money;
}

bool Economy::canAfford(int amount) const
{
    return amount >= 0 && money >= amount;
}

void Economy::addMoney(int amount)
{
    if (amount > 0)
    {
        money += amount;
    }
}

bool Economy::spendMoney(int amount)
{
    if (!canAfford(amount))
    {
        return false;
    }

    money -= amount;
    return true;
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

float Economy::getMaintenanceForTile(TileType type)
{
    switch (type)
    {
        case TileType::Road:
            return ROAD_MAINTENANCE;
        case TileType::Residential:
            return RESIDENTIAL_MAINTENANCE;
        case TileType::Commercial:
            return COMMERCIAL_MAINTENANCE;
        case TileType::Industrial:
            return INDUSTRIAL_MAINTENANCE;
        case TileType::Park:
            return PARK_MAINTENANCE;
        case TileType::Grass:
            return 0.0f;
    }

    return 0.0f;
}

bool Economy::canAfford(TileType type) const
{
    return canAfford(getCost(type));
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
    if (!spendMoney(getCost(type)))
    {
        return false;
    }

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

void Economy::update(const World& world, const Population& population,
                     const urbania::Employment& employment, float simulationDeltaTime)
{
    // Zero/negative deltas (paused) accumulate nothing: no per-frame
    // income, settlement happens only on full simulation days.
    if (simulationDeltaTime <= 0.0f)
    {
        return;
    }

    secondsTowardNextDay += simulationDeltaTime;
    while (secondsTowardNextDay >= SIM_SECONDS_PER_DAY)
    {
        secondsTowardNextDay -= SIM_SECONDS_PER_DAY;
        settleDay(world, population, employment);
    }
}

float Economy::getTaxIncome() const
{
    return taxIncome;
}

float Economy::getMaintenanceCost() const
{
    return maintenanceCost;
}

float Economy::getNetIncome() const
{
    return netIncome;
}

float Economy::getTotalTaxCollected() const
{
    return totalTaxCollected;
}

float Economy::getTotalMaintenancePaid() const
{
    return totalMaintenancePaid;
}

void Economy::settleDay(const World& world, const Population& population,
                        const urbania::Employment& employment)
{
    // Maintenance from currently existing tiles (counted live, no extra
    // Tile fields).
    float maintenance = 0.0f;
    for (int y = 0; y < world.getHeight(); ++y)
    {
        for (int x = 0; x < world.getWidth(); ++x)
        {
            maintenance += getMaintenanceForTile(world.getTile(x, y).type);
        }
    }

    // Citizen tax from every active citizen.
    const float citizenTax =
        static_cast<float>(population.getTotalPopulation()) * RESIDENTIAL_TAX_PER_CITIZEN;

    // Workplace tax from occupied jobs only, typed by the live World tile
    // under each job (no duplicated job data).
    float commercialJobs = 0.0f;
    float industrialJobs = 0.0f;
    for (const urbania::Job& job : employment.getJobs())
    {
        if (!job.occupied || !job.workplace.valid)
        {
            continue;
        }
        if (job.workplace.x < 0 || job.workplace.y < 0 || job.workplace.x >= world.getWidth() ||
            job.workplace.y >= world.getHeight())
        {
            continue;
        }
        const TileType type = world.getTile(job.workplace.x, job.workplace.y).type;
        if (type == TileType::Commercial)
        {
            commercialJobs += 1.0f;
        }
        else if (type == TileType::Industrial)
        {
            industrialJobs += 1.0f;
        }
    }

    const float tax = citizenTax + commercialJobs * COMMERCIAL_TAX_PER_JOB +
                      industrialJobs * INDUSTRIAL_TAX_PER_JOB;
    const float net = tax - maintenance;

    taxIncome = tax;
    maintenanceCost = maintenance;
    netIncome = net;
    totalTaxCollected += tax;
    totalMaintenancePaid += maintenance;

    // Net Income = Tax Income - Maintenance; Money += Net Income.
    // Construction stays an immediate expense elsewhere. Money never
    // goes negative: clamp at zero and keep the deficit visible via the
    // daily net-income statistic. No bankruptcy mechanics yet.
    const int netRounded = static_cast<int>(net);
    if (money + netRounded < 0)
    {
        money = 0;
    }
    else
    {
        money = std::max(0, money + netRounded);
    }
}
