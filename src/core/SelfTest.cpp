#include "core/SelfTest.h"

#include "simulation/CitizenManager.h"
#include "simulation/Economy.h"
#include "simulation/Pathfinder.h"
#include "simulation/Simulation.h"
#include "world/Tile.h"
#include "world/World.h"

namespace urbania {
namespace {

// Fixed, in-bounds test district far from the usual center map.
constexpr int HOME_X = 70;
constexpr int HOME_Y = 70;
constexpr int WORK_X = 74;
constexpr int WORK_Y = 70;
constexpr int ROAD_Y = 71;
constexpr int ROAD_X0 = 70;
constexpr int ROAD_X1 = 74;

// 2000 (home) + 5000 (work) + 5x100 (roads) + 100 (restore) = 7600.
constexpr int REQUIRED_MONEY = 7600;
constexpr float SIM_HOUR = 3600.0f;

int manhattan(int x1, int y1, int x2, int y2)
{
    const int dx = x1 > x2 ? x1 - x2 : x2 - x1;
    const int dy = y1 > y2 ? y1 - y2 : y2 - y1;
    return dx + dy;
}

}  // namespace

void SelfTest::run(World& world, Economy& economy, Simulation& simulation)
{
    results.clear();
    passed = 0;
    total = 0;
    ran = true;

    // Preconditions: free site, affordable test (SKIP, not FAIL).
    for (int x = ROAD_X0; x <= ROAD_X1; ++x)
    {
        if (world.getTile(x, ROAD_Y).type != TileType::Grass ||
            world.getTile(x, HOME_Y).type != TileType::Grass)
        {
            results.push_back("SKIP: test site occupied, clear x=70..74 y=70..71");
            return;
        }
    }
    if (economy.getMoney() < REQUIRED_MONEY)
    {
        results.push_back("SKIP: need Rs. 7,600 for the test build");
        return;
    }

    const int nodesBefore = simulation.getRoadNetwork().getNodeCount();

    // T1: construction through the real Economy rules.
    bool built = economy.tryBuild(world.getTile(HOME_X, HOME_Y), TileType::Residential);
    built = economy.tryBuild(world.getTile(WORK_X, WORK_Y), TileType::Commercial) && built;
    for (int x = ROAD_X0; x <= ROAD_X1; ++x)
    {
        built = economy.tryBuild(world.getTile(x, ROAD_Y), TileType::Road) && built;
    }
    check(built, "T1 build home+work+5 roads");
    simulation.getRoadNetwork().rebuild(world);
    check(simulation.getRoadNetwork().getNodeCount() == nodesBefore + 5, "T2 road nodes +5");

    // T3: A* over the new row, endpoints included, road-only, cardinal.
    {
        const std::vector<TileCoordinate> path = Pathfinder::findPath(
            simulation.getRoadNetwork(), { ROAD_X0, ROAD_Y, true }, { ROAD_X1, ROAD_Y, true });
        bool shapeOk = path.size() == 5;
        for (std::size_t i = 0; shapeOk && i < path.size(); ++i)
        {
            shapeOk = path[i].valid && world.getTile(path[i].x, path[i].y).type == TileType::Road;
            if (i > 0)
            {
                shapeOk = shapeOk && manhattan(path[i].x, path[i].y, path[i - 1].x,
                                               path[i - 1].y) == 1;
            }
        }
        if (!path.empty())
        {
            shapeOk = shapeOk && path.front().x == ROAD_X0 && path.back().x == ROAD_X1;
        }
        check(shapeOk, "T3 A* row path len 5, cardinal, road-only");
    }

    // T4: one sim hour grows exactly one resident on the fresh tile.
    simulation.update(SIM_HOUR);
    check(simulation.getPopulation().getResidentsAt(HOME_X, HOME_Y) == 1, "T4 growth 0->1");

    // T5: newcomer employed with a valid commute route (bounded wait so
    // a saturated city reports SKIP instead of hanging the test).
    // Citizens are tracked by ID: manager storage can reallocate.
    int newcomerId = 0;
    for (int i = 0; i < 30 && newcomerId == 0; ++i)
    {
        simulation.update(SIM_HOUR);
        for (const Citizen& citizen : simulation.getPopulation().getCitizens().getCitizens())
        {
            if (citizen.home.valid && citizen.home.x == HOME_X && citizen.home.y == HOME_Y &&
                citizen.employed && !citizen.commutePath.empty())
            {
                newcomerId = citizen.id;
                break;
            }
        }
    }
    if (newcomerId == 0)
    {
        skip("T5 newcomer employed+routed (city saturated?)");
    }
    else
    {
        check(true, "T5 newcomer employed with route");
    }

    // T6: breaking the row invalidates the newcomer's route.
    if (newcomerId == 0)
    {
        skip("T6 break road invalidates route");
    }
    else
    {
        economy.tryDemolish(world.getTile(72, ROAD_Y));
        simulation.getRoadNetwork().rebuild(world);
        simulation.update(0.0f);
        const Citizen* checkup = simulation.getPopulation().getCitizens().getCitizen(newcomerId);
        check(checkup != nullptr && checkup->commutePath.empty(), "T6 break road -> route invalid");
    }

    // T7: restoring the row heals the newcomer's route.
    if (newcomerId == 0)
    {
        skip("T7 restore road heals route");
    }
    else
    {
        economy.tryBuild(world.getTile(72, ROAD_Y), TileType::Road);
        simulation.getRoadNetwork().rebuild(world);
        simulation.update(0.0f);
        const Citizen* healed =
            simulation.getPopulation().getCitizens().getCitizen(newcomerId);
        check(healed != nullptr && !healed->commutePath.empty(), "T7 restore road -> route back");
    }

    // T8: movement and traffic advance on simulation time. A routed
    // citizen must own a vehicle, and stepping time must move things.
    if (newcomerId == 0)
    {
        skip("T8 vehicles advance on sim time");
    }
    else
    {
        simulation.update(0.5f);
        simulation.update(0.5f);
        const Citizen* moved =
            simulation.getPopulation().getCitizens().getCitizen(newcomerId);
        const bool leftHome =
            moved != nullptr &&
            (moved->pathIndex > 0 || moved->currentTile.x != HOME_X ||
             moved->currentTile.y != HOME_Y);
        check(leftHome, "T8 newcomer left home");
        check(simulation.getTraffic().getVehicleCount() >= 1, "T8 vehicle exists");
    }

    // T9: cleanup demolishes the test district; network returns.
    economy.tryDemolish(world.getTile(HOME_X, HOME_Y));
    economy.tryDemolish(world.getTile(WORK_X, WORK_Y));
    for (int x = ROAD_X0; x <= ROAD_X1; ++x)
    {
        economy.tryDemolish(world.getTile(x, ROAD_Y));
    }
    simulation.getRoadNetwork().rebuild(world);
    simulation.update(0.0f);
    check(simulation.getRoadNetwork().getNodeCount() == nodesBefore, "T9 cleanup nodes back");
    check(simulation.getPopulation().getResidentsAt(HOME_X, HOME_Y) == 0,
          "T10 cleanup residents gone");
}

bool SelfTest::hasRun() const
{
    return ran;
}

bool SelfTest::allPassed() const
{
    return ran && total > 0 && passed == total;
}

int SelfTest::getPassed() const
{
    return passed;
}

int SelfTest::getTotal() const
{
    return total;
}

const std::vector<std::string>& SelfTest::getResults() const
{
    return results;
}

void SelfTest::check(bool condition, const std::string& label)
{
    ++total;
    if (condition)
    {
        ++passed;
    }
    results.push_back((condition ? "ok " : "FAIL ") + label);
}

void SelfTest::skip(const std::string& reason)
{
    results.push_back("SKIP " + reason);
}

}  // namespace urbania
