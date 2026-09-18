#pragma once

#include <string>
#include <vector>

class Economy;
class Simulation;
class World;

namespace urbania {

// Temporary development self-test, run on demand with F9. Drives the
// REAL game systems (World, Economy, Simulation) through scripted
// checks using direct calls and fixed simulation deltas — never the
// OS mouse — so results are immune to cursor interference and fully
// deterministic for a given city state.
//
// The test builds a small district, verifies growth, employment,
// routing, movement, and traffic, then demolishes its own buildings
// (no refund, same as manual play). Costs real money (see
// REQUIRED_MONEY) and advances simulation time; preconditions that
// the live city cannot meet report SKIP, never false FAILs.
class SelfTest {
public:
    void run(World& world, Economy& economy, Simulation& simulation);

    bool hasRun() const;
    bool allPassed() const;
    int getPassed() const;
    int getTotal() const;
    const std::vector<std::string>& getResults() const;

private:
    void check(bool condition, const std::string& label);
    void skip(const std::string& reason);

    std::vector<std::string> results;
    int passed = 0;
    int total = 0;
    bool ran = false;
};

}  // namespace urbania
