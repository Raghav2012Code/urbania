#include "simulation/Traffic.h"

#include <algorithm>

#include "simulation/CitizenManager.h"
#include "simulation/Congestion.h"
#include "simulation/RoadNetwork.h"

namespace urbania {

void Traffic::update(const RoadNetwork& roadNetwork, CitizenManager& citizens,
                     float simulationDeltaTime, const Congestion& congestion)
{
    sweepInactive();
    spawnFromCommuters(citizens);
    moveVehicles(roadNetwork, citizens, simulationDeltaTime, congestion);
}

int Traffic::getVehicleCount() const
{
    return static_cast<int>(vehicles.size());
}

int Traffic::getActiveVehicleCount() const
{
    int count = 0;
    for (const Vehicle& vehicle : vehicles)
    {
        if (vehicle.active)
        {
            ++count;
        }
    }
    return count;
}

const std::vector<Vehicle>& Traffic::getVehicles() const
{
    return vehicles;
}

void Traffic::clear()
{
    vehicles.clear();
    citizenToVehicle.clear();
    nextVehicleId = 1;
}

const std::vector<TileCoordinate>& Traffic::getRepresentativeRoute() const
{
    for (const Vehicle& vehicle : vehicles)
    {
        if (vehicle.active && !vehicle.path.empty())
        {
            return vehicle.path;
        }
    }

    static const std::vector<TileCoordinate> empty{};
    return empty;
}

void Traffic::sweepInactive()
{
    std::vector<Vehicle> kept;
    kept.reserve(vehicles.size());
    for (const Vehicle& vehicle : vehicles)
    {
        if (vehicle.active)
        {
            kept.push_back(vehicle);
        }
        else
        {
            citizenToVehicle.erase(vehicle.citizenId);
        }
    }
    vehicles = kept;
}

void Traffic::spawnFromCommuters(CitizenManager& citizens)
{
    // One vehicle per employed citizen with a travelable route.
    // The citizenId map blocks duplicates; single-node routes have
    // nothing to drive, so they never spawn.
    for (const Citizen& citizen : citizens.getCitizens())
    {
        if (!citizen.employed || citizen.commutePath.size() < 2)
        {
            continue;
        }
        if (citizenToVehicle.find(citizen.id) != citizenToVehicle.end())
        {
            continue;
        }

        Vehicle vehicle;
        vehicle.id = nextVehicleId;
        vehicle.citizenId = citizen.id;
        vehicle.path = citizen.commutePath;
        // Start where the citizen actually is. Defaulting to 0 would
        // teleport the car back to the start of the route whenever the
        // citizen is part-way through a trip that was re-planned.
        vehicle.pathIndex =
            std::clamp(citizen.pathIndex, 0, static_cast<int>(vehicle.path.size()) - 1);
        vehicle.movementProgress = citizen.movementProgress;
        vehicle.speed = VEHICLE_SPEED_TILES_PER_SECOND;
        vehicle.active = true;
        ++nextVehicleId;

        vehicles.push_back(vehicle);
        citizenToVehicle[citizen.id] = vehicle.id;
    }
}

void Traffic::moveVehicles(const RoadNetwork& roadNetwork, CitizenManager& citizens,
                           float simulationDeltaTime, const Congestion& congestion)
{
    for (Vehicle& vehicle : vehicles)
    {
        if (!vehicle.active)
        {
            continue;
        }

        Citizen* citizen = citizens.getCitizen(vehicle.citizenId);
        if (citizen == nullptr || !citizen->employed || citizen->commutePath.empty())
        {
            vehicle.active = false;
            continue;
        }

        // A demolished road anywhere on the remaining route stops the
        // vehicle safely instead of driving onto grass.
        bool routeValid = true;
        for (const TileCoordinate& step : vehicle.path)
        {
            if (!step.valid || !roadNetwork.isRoad(step.x, step.y))
            {
                routeValid = false;
                break;
            }
        }
        if (!routeValid)
        {
            vehicle.active = false;
            continue;
        }

        const int legs = static_cast<int>(vehicle.path.size()) - 1;

        // Congestion from the previous update slows this step down.
        // Using last frame's values (rather than recomputing mid-move)
        // keeps the feedback loop to one clean pass per update.
        int tileIndex = vehicle.pathIndex;
        if (tileIndex < 0)
        {
            tileIndex = 0;
        }
        if (tileIndex > legs)
        {
            tileIndex = legs;
        }
        const TileCoordinate& current = vehicle.path[tileIndex];
        const float multiplier =
            Congestion::speedMultiplier(congestion.getCongestion(current.x, current.y));

        vehicle.movementProgress += simulationDeltaTime * vehicle.speed * multiplier;

        // Shuttle along the route: reaching either end reverses direction
        // instead of retiring the vehicle. This is what keeps one Vehicle
        // per commuter for the whole session rather than allocating a new
        // one on every completed trip.
        while (vehicle.movementProgress >= 1.0f)
        {
            if (vehicle.direction > 0)
            {
                if (vehicle.pathIndex < legs)
                {
                    ++vehicle.pathIndex;
                }
                else
                {
                    vehicle.direction = -1;
                }
            }
            else
            {
                if (vehicle.pathIndex > 0)
                {
                    --vehicle.pathIndex;
                }
                else
                {
                    vehicle.direction = 1;
                }
            }

            if (vehicle.direction > 0 && vehicle.pathIndex >= legs)
            {
                vehicle.direction = -1;
            }
            if (vehicle.direction < 0 && vehicle.pathIndex <= 0)
            {
                vehicle.direction = 1;
            }

            vehicle.movementProgress -= 1.0f;
        }
    }
}

}  // namespace urbania
