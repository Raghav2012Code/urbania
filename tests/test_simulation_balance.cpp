#include <iostream>
#include <cmath>

#include "test_check.h"

#include "world/World.h"
#include "world/Tile.h"
#include "simulation/Demand.h"
#include "simulation/Population.h"
#include "simulation/Employment.h"
#include "simulation/Economy.h"
#include "simulation/Simulation.h"

using namespace urbania;

void test_demand_system()
{
    World world;
    Population population;
    Employment employment;
    Demand demand;

    // 1. Empty city with no housing: neutral demand (0)
    demand.recalculate(world, population, employment);
    CHECK(demand.getResidentialDemand() == 0);
    CHECK(demand.getCommercialDemand() == 0);
    CHECK(demand.getIndustrialDemand() == 0);

    // 2. Build 1 residential tile (capacity 10), population is 0 -> Occupancy 0.0 -> Demand -100
    world.getTile(5, 5).type = TileType::Residential;
    population.update(world, 0.0f);
    demand.recalculate(world, population, employment);
    CHECK(demand.getResidentialDemand() == -100);

    // 3. Populate with 5 citizens (half capacity 5/10 = 0.5) -> Demand should be 0 (neutral)
    for (int i = 0; i < 5; ++i)
    {
        population.getCitizenManager().createCitizen({ 5, 5, true });
    }
    population.update(world, 0.0f);
    demand.recalculate(world, population, employment);
    CHECK(demand.getResidentialDemand() == 0);

    // 4. Populate with 5 more citizens (full capacity 10/10 = 1.0) -> Demand should be +100 (max)
    for (int i = 0; i < 5; ++i)
    {
        population.getCitizenManager().createCitizen({ 5, 5, true });
    }
    population.update(world, 0.0f);
    demand.recalculate(world, population, employment);
    CHECK(demand.getResidentialDemand() == 100);

    // 5. Commercial and Industrial demand when citizens exist without jobs
    // 10 citizens -> commercial & industrial demand should grow to +100
    CHECK(demand.getCommercialDemand() == 100);
    CHECK(demand.getIndustrialDemand() == 100);
}

void test_population_growth_and_capacity()
{
    World world;
    Population population;

    // Build 2 residential tiles (total capacity 20)
    world.getTile(10, 10).type = TileType::Residential;
    world.getTile(11, 10).type = TileType::Residential;

    population.update(world, 0.0f);
    CHECK(population.getTotalPopulation() == 0);
    CHECK(population.getTotalHousingCapacity() == 20);

    // Advance simulation by 1 sim hour (3600 seconds)
    // Each residential tile grows by 1 resident per sim hour
    population.update(world, 3600.0f);
    CHECK(population.getTotalPopulation() == 2);
    CHECK(population.getResidentsAt(10, 10) == 1);
    CHECK(population.getResidentsAt(11, 10) == 1);

    // Advance by 9 more sim hours (total 10 hours)
    for (int i = 0; i < 9; ++i)
    {
        population.update(world, 3600.0f);
    }
    CHECK(population.getTotalPopulation() == 20);
    CHECK(population.getResidentsAt(10, 10) == 10);
    CHECK(population.getResidentsAt(11, 10) == 10);

    // Advance further: capacity should stay capped at 10 per tile (total 20)
    population.update(world, 3600.0f);
    CHECK(population.getTotalPopulation() == 20);

    // Demolish one residential tile -> its 10 citizens are removed
    world.getTile(11, 10).type = TileType::Grass;
    population.update(world, 0.0f);
    CHECK(population.getTotalPopulation() == 10);
    CHECK(population.getTotalHousingCapacity() == 10);
    CHECK(population.getResidentsAt(10, 10) == 10);
    CHECK(population.getResidentsAt(11, 10) == 0);
}

void test_profit_loss_and_ledger()
{
    World world;
    Population population;
    Employment employment;
    Economy economy;

    // 1. Initial State: Starting Money ₹100,000, no tiles built
    CHECK(economy.getMoney() == 100000);
    economy.recalculate(world, population, employment);
    // Base municipal utility upkeep is ₹1,100/day
    CHECK(economy.getMaintenanceCost() == 1100.0f);
    CHECK(economy.getTaxIncome() == 0.0f);
    CHECK(economy.getNetIncome() == -1100.0f);

    // 2. Build 10 Roads (₹100 each = ₹1,000) and 1 Commercial zone (₹5,000)
    for (int x = 0; x < 10; ++x)
    {
        CHECK(economy.tryBuild(world.getTile(x, 0), TileType::Road));
    }
    CHECK(economy.tryBuild(world.getTile(0, 1), TileType::Commercial));

    // Wallet deducted immediately: ₹100,000 - ₹1,000 - ₹5,000 = ₹94,000
    CHECK(economy.getMoney() == 94000);

    // Maintenance increases:
    // Utility (₹1100) + 10 Roads * ₹2 (₹20) + 1 Commercial * ₹10 (₹10) = ₹1,130
    economy.recalculate(world, population, employment);
    CHECK(economy.getMaintenanceCost() == 1130.0f);
    CHECK(economy.getNetIncome() == -1130.0f);

    // 3. Add 20 citizens in 2 residential homes (₹100/citizen = ₹2,000 tax)
    // and sync employment for 8 commercial jobs
    world.getTile(0, 2).type = TileType::Residential;
    world.getTile(1, 2).type = TileType::Residential;
    population.update(world, 0.0f);
    for (int i = 0; i < 20; ++i)
    {
        population.getCitizenManager().createCitizen({ i < 10 ? 0 : 1, 2, true });
    }
    population.update(world, 0.0f);
    employment.update(world, population.getCitizenManager(), 0.0f);

    // 20 citizens * ₹100 = ₹2,000 citizen tax
    // 8 commercial occupied jobs * ₹50 = ₹400 workplace tax
    // Total Tax Income = ₹2,400
    // Maintenance = ₹1,130 + 2 Residential * ₹5 (₹10) = ₹1,140
    // Net Income = ₹2,400 - ₹1,140 = +₹1,260 / day profit!
    economy.recalculate(world, population, employment);
    CHECK(economy.getTaxIncome() == 2400.0f);
    CHECK(economy.getMaintenanceCost() == 1140.0f);
    CHECK(economy.getNetIncome() == 1260.0f);

    // Settle 1 full simulation day (86,400 seconds)
    const int moneyBefore = economy.getMoney();
    economy.update(world, population, employment, 86400.0f);
    CHECK(economy.getMoney() == moneyBefore + 1260);
}

// #4: the flat resident-count cache must stay consistent with the citizen
// roster across growth and demolition.
void test_resident_counts_consistent_with_population()
{
    World world;
    const int homeCoords[6][2] = {
        { 10, 10 }, { 11, 10 }, { 12, 10 }, { 20, 20 }, { 21, 20 }, { 30, 30 }
    };
    for (const auto& home : homeCoords)
    {
        world.getTile(home[0], home[1]).type = TileType::Residential;
    }

    Population population;
    for (int hour = 0; hour < 25; ++hour)
    {
        population.update(world, 3600.0f);
    }

    int summed = 0;
    for (const auto& home : homeCoords)
    {
        const int residents = population.getResidentsAt(home[0], home[1]);
        CHECK(residents >= 0 && residents <= Population::RESIDENTS_PER_RESIDENTIAL_TILE);
        summed += residents;
    }
    CHECK(summed == population.getTotalPopulation());
    CHECK(population.getTotalPopulation() == 6 * Population::RESIDENTS_PER_RESIDENTIAL_TILE);
    CHECK(population.getResidentsAt(0, 0) == 0);
}

// #5: gating the world-derived job rebuild on world edits must not change
// employment behaviour across demolition and reconstruction.
void test_employment_reconcile_on_world_change()
{
    World world;
    world.getTile(0, 0).type = TileType::Commercial;
    world.getTile(0, 1).type = TileType::Residential;

    Simulation simulation;
    simulation.initialize(world);
    for (int i = 0; i < 10; ++i)
    {
        simulation.getPopulation().getCitizenManager().createCitizen({ 0, 1, true });
    }
    simulation.update(0.0f);

    CHECK(simulation.getEmployment().getTotalJobs() == 8);
    CHECK(simulation.getEmployment().getEmployedCitizens() == 8);
    CHECK(simulation.getEmployment().getUnemployedCitizens() == 2);

    // Demolish the only workplace: all jobs vanish, everyone is unemployed.
    CHECK(simulation.getEconomy().tryDemolish(world.getTile(0, 0)));
    simulation.onWorldModified();
    CHECK(simulation.getEmployment().getTotalJobs() == 0);
    CHECK(simulation.getEmployment().getEmployedCitizens() == 0);
    CHECK(simulation.getEmployment().getUnemployedCitizens() == 10);

    // Rebuild it as industrial (15 jobs): all ten rematch.
    CHECK(simulation.getEconomy().tryBuild(world.getTile(0, 0), TileType::Industrial));
    simulation.onWorldModified();
    CHECK(simulation.getEmployment().getTotalJobs() == 15);
    CHECK(simulation.getEmployment().getEmployedCitizens() == 10);
}

int main()
{
    std::cout << "=== Running Simulation Balance Unit Tests ===\n";
    test_demand_system();
    test_population_growth_and_capacity();
    test_profit_loss_and_ledger();
    test_resident_counts_consistent_with_population();
    test_employment_reconcile_on_world_change();
    return testcheck::summary("Simulation Balance Unit Tests");
}
