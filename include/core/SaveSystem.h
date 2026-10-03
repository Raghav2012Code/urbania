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
// Layout (v2):
//   URBANIA_SAVE 2
//   WORLD <w> <h> + <h> rows of <w> tile chars (G/R/H/C/I/P)
//   CLOCK <simTime> <timeScale> <paused01>
//   ECONOMY <money> <totalTax> <totalMaint> <secondsTowardDay>
//   SIM <elapsedSeconds>
//   HOURCLOCKS <landValue> <housing> <happiness> <demand>   (v2+)
//   UTILCAPS <elec> <water> <sewage>
//   CITIZENS <count> <nextId> + per citizen CIT/CPATH records
//   POPGROWTH <count> + PG records
//   POLLUTION <count> <seconds> <avg> <max> + POL records
//   STOPS <count> <nextStop> + STOP records
//   ROUTES <count> <nextRoute> <nextBus> + ROUTE records
//   CAMERA <tx> <ty> <zoom>
//   END
//
// v1 files are still accepted: they simply lack HOURCLOCKS and those four
// hourly accumulators start at zero.
//
// Load is transactional: the file is fully parsed and validated into
// memory first; the live city is only mutated when everything checks
// out. Corrupt files keep the current city intact and never crash.
class SaveSystem {
public:
    static constexpr int SAVE_VERSION = 2;
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

#ifdef URBANIA_ENABLE_LOAD_FAILPOINT
    // Test-only seam: when armed, the next load throws once during the apply
    // phase, after the world tiles are committed, so a test can prove a failed
    // load restores the live city. This symbol and the macro are absent from
    // normal builds.
    static void setLoadFailpointForTests(bool armed);
#endif
};

}  // namespace urbania
