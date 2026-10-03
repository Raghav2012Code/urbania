#include <iostream>
#include <limits>

#include "test_check.h"

#include "world/World.h"
#include "world/Tile.h"
#include "simulation/Population.h"
#include "simulation/Employment.h"
#include "simulation/Economy.h"

using namespace urbania;

// Issue #3: the daily settlement used to overflow a 32-bit int before the
// bounds were applied, wrapping negative and taking the clamp-to-zero branch,
// silently wiping the treasury. Arithmetic is now 64-bit and clamped.
void test_settlement_near_int_max_does_not_wipe()
{
    World world;
    Population population;
    Employment employment;
    Economy economy;

    // Positive net income: 20 citizens in 2 homes (Rs. 2,000 tax) against the
    // fixed utility upkeep plus tile maintenance.
    world.getTile(0, 2).type = TileType::Residential;
    world.getTile(1, 2).type = TileType::Residential;
    population.update(world, 0.0f);
    for (int i = 0; i < 20; ++i)
    {
        population.getCitizenManager().createCitizen({ i < 10 ? 0 : 1, 2, true });
    }
    population.update(world, 0.0f);
    employment.update(world, population.getCitizenManager(), 0.0f);

    // Net income here is ~+1,260/day, so seed the wallet with less headroom
    // than that: the pre-fix 32-bit `money + net` overflows, wraps negative,
    // and takes the clamp-to-zero branch.
    const int nearMax = std::numeric_limits<int>::max() - 100;
    economy.restoreSavedState(nearMax, 0.0f, 0.0f, 0.0f);
    CHECK(economy.getMoney() == nearMax);

    economy.update(world, population, employment, 86400.0f);

    // Clamped at INT_MAX, not wrapped to zero.
    CHECK(economy.getMoney() == std::numeric_limits<int>::max());
    CHECK(economy.getMoney() > 0);
}

// restoreSavedState clamps both ends; a negative or out-of-range save value
// must not arm the next settlement.
void test_restore_clamps_money()
{
    Economy economy;

    economy.restoreSavedState(-1, 0.0f, 0.0f, 0.0f);
    CHECK(economy.getMoney() == 0);

    economy.restoreSavedState(std::numeric_limits<int>::max(), 0.0f, 0.0f, 0.0f);
    CHECK(economy.getMoney() == std::numeric_limits<int>::max());
}

int main()
{
    std::cout << "=== Running Economy Regression Tests ===\n";
    test_settlement_near_int_max_does_not_wipe();
    test_restore_clamps_money();
    return testcheck::summary("Economy Regression Tests");
}
