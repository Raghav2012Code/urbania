#pragma once

#include <map>
#include <vector>

#include "simulation/Vehicle.h"

namespace urbania {

class CitizenManager;
class Congestion;
class RoadNetwork;

// Owns all Vehicles: one road vehicle per employed citizen with a valid
// commute route. Spawning is deduplicated by citizen (citizenId to
// vehicleId map), so a citizen never owns two vehicles at once.
//
// Each Vehicle shuttles along its route, reversing at either end, so it
// serves its commuter for the whole session and nextVehicleId stays
// bounded by the number of commuters. Vehicles are only deactivated when
// their citizen is gone, loses employment, or its route breaks (demolished
// road); those are swept at the next update and respawn deterministically
// once a valid route exists again. Deterministic: spawn order follows
// citizen ID order. No raylib dependency here.
class Traffic {
public:
    static constexpr float VEHICLE_SPEED_TILES_PER_SECOND = 4.0f;

    void update(const RoadNetwork& roadNetwork, CitizenManager& citizens,
                float simulationDeltaTime, const Congestion& congestion);

    int getVehicleCount() const;
    int getActiveVehicleCount() const;
    const std::vector<Vehicle>& getVehicles() const;
    const std::vector<TileCoordinate>& getRepresentativeRoute() const;

    // Drops all vehicles (transient trip state). Used after load; trips
    // respawn deterministically from commute routes. No file I/O here.
    void clear();

private:
    void sweepInactive();
    void spawnFromCommuters(CitizenManager& citizens);
    void moveVehicles(const RoadNetwork& roadNetwork, CitizenManager& citizens,
                      float simulationDeltaTime, const Congestion& congestion);

    std::vector<Vehicle> vehicles;
    std::map<int, int> citizenToVehicle;
    int nextVehicleId = 1;
};

}  // namespace urbania
