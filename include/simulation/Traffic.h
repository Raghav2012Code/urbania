#pragma once

#include <map>
#include <vector>

#include "simulation/Vehicle.h"

namespace urbania {

class CitizenManager;
class Congestion;
class RoadNetwork;

// Owns all Vehicles: one road trip per employed citizen with a valid
// commute route. Spawning is deduplicated by citizen (citizenId to
// vehicleId map), so a citizen never owns two vehicles at once; after
// a trip finishes and is swept, the same citizen may start another
// trip later, but never a new vehicle every frame.
//
// Finished vehicles are marked inactive and swept at the next update;
// demolished roads safely deactivate affected vehicles. Deterministic:
// spawn order follows citizen ID order. No raylib dependency here.
class Traffic {
public:
    static constexpr float VEHICLE_SPEED_TILES_PER_SECOND = 4.0f;

    void update(const RoadNetwork& roadNetwork, CitizenManager& citizens,
                float simulationDeltaTime, const Congestion& congestion);

    int getVehicleCount() const;
    int getActiveVehicleCount() const;
    const std::vector<Vehicle>& getVehicles() const;
    const std::vector<TileCoordinate>& getRepresentativeRoute() const;

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
