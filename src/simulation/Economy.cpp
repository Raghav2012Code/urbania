#include "simulation/Economy.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "simulation/Employment.h"
#include "simulation/Population.h"
#include "simulation/Utilities.h"
#include "world/World.h"

static_assert(Economy::UTILITY_MAINTENANCE ==
                  urbania::Utilities::TOTAL_DAILY_MAINTENANCE,
              "Economy utility upkeep must match Utilities maintenance");

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
    // Honour the sign (a refund is a negative add) and clamp both ends the
    // same way settleDay does: 64-bit arithmetic so a near-INT_MAX wallet
    // cannot overflow, and no way to drive money negative.
    const long long next = static_cast<long long>(money) + amount;
    money = static_cast<int>(std::clamp<long long>(next, 0,
                                                   static_cast<long long>(
                                                       std::numeric_limits<int>::max())));
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

float Economy::getUtilityMaintenanceCost() const
{
    return utilityMaintenance;
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

float Economy::getSecondsTowardNextDay() const
{
    return secondsTowardNextDay;
}

void Economy::restoreSavedState(int money_, float totalTax_, float totalMaint_, float secondsToward_)
{
    // Clamp both ends: a negative value is invalid, and an out-of-range
    // positive one would overflow the next daily settlement.
    money = std::clamp(money_, 0, std::numeric_limits<int>::max());
    totalTaxCollected = totalTax_ < 0.0f ? 0.0f : totalTax_;
    totalMaintenancePaid = totalMaint_ < 0.0f ? 0.0f : totalMaint_;
    secondsTowardNextDay = secondsToward_ < 0.0f ? 0.0f
        : (secondsToward_ >= SIM_SECONDS_PER_DAY ? 0.0f : secondsToward_);
}

void Economy::recalculate(const World& world, const Population& population,
                          const urbania::Employment& employment)
{
    float maintenance = UTILITY_MAINTENANCE;
    for (int y = 0; y < world.getHeight(); ++y)
    {
        for (int x = 0; x < world.getWidth(); ++x)
        {
            maintenance += getMaintenanceForTile(world.getTile(x, y).type);
        }
    }

    const float citizenTax =
        static_cast<float>(population.getTotalPopulation()) * RESIDENTIAL_TAX_PER_CITIZEN;

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
    utilityMaintenance = UTILITY_MAINTENANCE;
    maintenanceCost = maintenance;
    netIncome = net;
}

void Economy::settleDay(const World& world, const Population& population,
                        const urbania::Employment& employment)
{
    recalculate(world, population, employment);

    totalTaxCollected += taxIncome;
    totalMaintenancePaid += maintenanceCost;

    // Arithmetic in 64-bit: money is a 32-bit int and netIncome is a float,
    // so money + netRounded can overflow before the bounds are applied. An
    // overflowing sum wraps negative and would take the clamp-to-zero branch,
    // silently wiping the treasury.
    const long long netRounded = std::llround(static_cast<double>(netIncome));
    const long long next = static_cast<long long>(money) + netRounded;
    money = static_cast<int>(std::clamp<long long>(next, 0,
                                                   static_cast<long long>(
                                                       std::numeric_limits<int>::max())));
}
