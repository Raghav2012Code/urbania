#pragma once

#include <string>

class World;
class Simulation;
class SimulationClock;

namespace urbania {
class Camera;

// Versioned city save/load. Custom line-based text format (no JSON
// dependency). All file I/O lives here; World/Simulation/rendering
// classes only expose small restoreSaved*() primitives and stay I/O free.
//
// Layout (v1):
//   URBANIA_SAVE 1
//   WORLD <w> <h> + <h> rows of <w> tile chars (G/R/H/C/I/P)
//   CLOCK <simTime> <timeScale> <paused01>
//   ECONOMY <money> <totalTax> <totalMaint> <secondsTowardDay>
//   SIM <elapsedSeconds>
//   UTILCAPS <elec> <water> <sewage>
//   CITIZENS <count> <nextId> + per citizen CIT/CPATH records
//   POPGROWTH <count> + PG records
//   POLLUTION <count> <seconds> <avg> <max> + POL records
//   STOPS <count> <nextStop> + STOP records
//   ROUTES <count> <nextRoute> <nextBus> + ROUTE records
//   CAMERA <tx> <ty> <zoom>
//   END
//
// Load is transactional: the file is fully parsed and validated into
// memory first; the live city is only mutated when everything checks
// out. Corrupt files keep the current city intact and never crash.
class SaveSystem {
public:
    static constexpr int SAVE_VERSION = 1;
    static constexpr const char* DEFAULT_PATH = "saves/urbania_save.dat";

    struct Result {
        bool ok = false;
        std::string message;
    };

    static Result save(const World& world, const Simulation& simulation,
                       const SimulationClock& clock, const Camera& camera,
                       const std::string& path = DEFAULT_PATH);

    static Result load(World& world, Simulation& simulation, SimulationClock& clock,
                       Camera& camera, const std::string& path = DEFAULT_PATH);
};

}  // namespace urbania
