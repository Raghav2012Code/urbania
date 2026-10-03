#include <iostream>
#include <vector>

#include "test_check.h"

#include "world/World.h"
#include "world/Tile.h"
#include "simulation/Utilities.h"
#include "simulation/Economy.h"
#include "simulation/Happiness.h"
#include "simulation/Population.h"
#include "simulation/Citizen.h"
#include "simulation/Employment.h"
#include "simulation/Pollution.h"
#include "simulation/Simulation.h"

using namespace urbania;

void test_demand_rates()
{
    int elec = 0, water = 0, sewage = 0;

    Utilities::getDemandsForType(TileType::Residential, elec, water, sewage);
    CHECK(elec == 1 && water == 1 && sewage == 1);

    Utilities::getDemandsForType(TileType::Commercial, elec, water, sewage);
    CHECK(elec == 2 && water == 2 && sewage == 2);

    Utilities::getDemandsForType(TileType::Industrial, elec, water, sewage);
    CHECK(elec == 4 && water == 3 && sewage == 3);

    Utilities::getDemandsForType(TileType::Road, elec, water, sewage);
    CHECK(elec == 0 && water == 0 && sewage == 0);

    Utilities::getDemandsForType(TileType::Park, elec, water, sewage);
    CHECK(elec == 0 && water == 0 && sewage == 0);

    Utilities::getDemandsForType(TileType::Grass, elec, water, sewage);
    CHECK(elec == 0 && water == 0 && sewage == 0);
}

void test_connection_radius()
{
    World world;

    // Place a road at (5, 5)
    world.getTile(5, 5).type = TileType::Road;

    // Connected tiles (within Chebyshev distance 2):
    // (5, 3) -> dy = 2 -> connected
    // (7, 5) -> dx = 2 -> connected
    // (7, 7) -> dx = 2, dy = 2 -> connected
    world.getTile(5, 3).type = TileType::Residential;
    world.getTile(7, 5).type = TileType::Commercial;
    world.getTile(7, 7).type = TileType::Industrial;

    // Disconnected tiles (Chebyshev distance > 2):
    // (5, 2) -> dy = 3 -> not connected
    // (8, 5) -> dx = 3 -> not connected
    // (8, 8) -> dx = 3, dy = 3 -> not connected
    world.getTile(5, 2).type = TileType::Residential;
    world.getTile(8, 5).type = TileType::Commercial;
    world.getTile(8, 8).type = TileType::Industrial;

    Utilities utils;
    utils.recalculate(world);

    CHECK(utils.isConnectedToRoad(world, 5, 3) == true);
    CHECK(utils.isConnectedToRoad(world, 7, 5) == true);
    CHECK(utils.isConnectedToRoad(world, 7, 7) == true);

    CHECK(utils.isConnectedToRoad(world, 5, 2) == false);
    CHECK(utils.isConnectedToRoad(world, 8, 5) == false);
    CHECK(utils.isConnectedToRoad(world, 8, 8) == false);

    CHECK(utils.isTileSupplied(5, 3) == true);
    CHECK(utils.isTileSupplied(7, 5) == true);
    CHECK(utils.isTileSupplied(7, 7) == true);

    CHECK(utils.isTileSupplied(5, 2) == false);
    CHECK(utils.isTileSupplied(8, 5) == false);
    CHECK(utils.isTileSupplied(8, 8) == false);
}

void test_capacity_and_deterministic_allocation()
{
    World world;

    // Continuous road along y = 0
    for (int x = 0; x < 50; ++x)
    {
        world.getTile(x, 0).type = TileType::Road;
    }

    // Place 60 commercial buildings along y = 1 (x: 0..49) and y = 2 (x: 0..9)
    // Total demand = 60 * 2 = 120 kW electricity, 120 kL water, 120 kL sewage
    // Capacity = 100 for all three.
    // 50 buildings should be fully supplied (50 * 2 = 100), exactly matching capacity.
    // The remaining 10 buildings should be unsupplied.
    for (int x = 0; x < 50; ++x)
    {
        world.getTile(x, 1).type = TileType::Commercial;
    }
    for (int x = 0; x < 10; ++x)
    {
        world.getTile(x, 2).type = TileType::Commercial;
    }

    Utilities utils;
    utils.recalculate(world);

    CHECK(utils.getTotalDevelopedBuildingCount() == 60);
    CHECK(utils.getElectricityDemand() == 120);
    CHECK(utils.getWaterDemand() == 120);
    CHECK(utils.getSewageDemand() == 120);

    CHECK(utils.getSuppliedBuildingCount() == 50);
    CHECK(utils.getUnsuppliedBuildingCount() == 10);

    // Row-major order check: row y=1 (x: 0..49) must be supplied
    for (int x = 0; x < 50; ++x)
    {
        CHECK(utils.isTileSupplied(x, 1) == true);
    }
    // row y=2 (x: 0..9) must be unsupplied because capacity was exhausted
    for (int x = 0; x < 10; ++x)
    {
        CHECK(utils.isTileSupplied(x, 2) == false);
    }
}

void test_happiness_impact()
{
    World world;

    // Home A at (5, 5) with adjacent road at (5, 4) -> Connected & Supplied
    world.getTile(5, 4).type = TileType::Road;
    world.getTile(5, 5).type = TileType::Residential;

    // Home B at (15, 15) with NO road nearby -> Unconnected & Unsupplied
    world.getTile(15, 15).type = TileType::Residential;

    Utilities utils;
    utils.recalculate(world);

    CHECK(utils.isTileSupplied(5, 5) == true);
    CHECK(utils.isTileSupplied(15, 15) == false);

    CitizenManager citizenManager;
    Citizen& citizenSupplied = citizenManager.createCitizen({ 5, 5, true });
    citizenSupplied.employed = true;
    citizenSupplied.commutePath = { { 5, 4, true }, { 5, 3, true } };

    Citizen& citizenUnsupplied = citizenManager.createCitizen({ 15, 15, true });
    citizenUnsupplied.employed = true;
    citizenUnsupplied.commutePath = { { 15, 14, true }, { 15, 13, true } };

    Pollution pollution;
    Happiness happiness;

    happiness.recalculate(world, citizenManager, pollution, utils);

    const float happyA = citizenManager.getCitizen(citizenSupplied.id)->happiness;
    const float happyB = citizenManager.getCitizen(citizenUnsupplied.id)->happiness;

    // Both are employed (+15), valid housing (+5), short commute (+0), same base (50), no pollution/parks.
    // happyA should have no utility penalty: 50 + 15 + 5 = 70.
    // happyB should have unpowered utility penalty (-20): 50 + 15 + 5 - 20 = 50.
    CHECK(happyA == 70.0f);
    CHECK(happyB == 50.0f);
    CHECK(happyA - happyB == 20.0f);
}

void test_economy_maintenance()
{
    World world;
    Economy economy;

    // Fixed daily utility upkeep: ₹1,100 (₹500 elec + ₹300 water + ₹300 sewage)
    CHECK(economy.getUtilityMaintenanceCost() == 1100.0f);

    // Initial money: ₹100,000
    const int startMoney = economy.getMoney();
    CHECK(startMoney == Economy::STARTING_MONEY);

    Population population;
    Employment employment;

    // Advance by 1 full day (86400 seconds)
    economy.update(world, population, employment, 86400.0f);

    // Money should decrease by daily maintenance (₹1,100)
    CHECK(economy.getMoney() == startMoney - 1100);
}

int main()
{
    std::cout << "=== Running City Utilities Unit Tests ===\n";
    test_demand_rates();
    test_connection_radius();
    test_capacity_and_deterministic_allocation();
    test_happiness_impact();
    test_economy_maintenance();
    return testcheck::summary("City Utilities Unit Tests");
}
