#include "simulation/CitizenMovement.h"

#include "simulation/CitizenManager.h"

namespace urbania {

void CitizenMovement::update(CitizenManager& citizens, float simulationDeltaTime)
{
    movingCitizens = 0;

    for (Citizen& citizen : citizens.getCitizens())
    {
        const int pathSize = static_cast<int>(citizen.commutePath.size());

        if (!citizen.employed || pathSize == 0)
        {
            // Stop and reset: back home, fresh journey state.
            citizen.currentTile = citizen.home;
            citizen.pathIndex = 0;
            citizen.movementProgress = 0.0f;
            citizen.commutingToWork = false;
            continue;
        }

        // A replaced (shorter) route clamps instead of teleporting.
        if (citizen.pathIndex > pathSize)
        {
            citizen.pathIndex = pathSize;
            citizen.movementProgress = 0.0f;
        }

        if (citizen.pathIndex >= pathSize)
        {
            // Arrived at the road tile adjacent to the workplace.
            citizen.commutingToWork = false;
            citizen.movementProgress = 0.0f;
            continue;
        }

        citizen.commutingToWork = true;
        citizen.movementProgress += simulationDeltaTime * TILES_PER_SIM_SECOND;

        while (citizen.movementProgress >= 1.0f && citizen.pathIndex < pathSize)
        {
            citizen.movementProgress -= 1.0f;
            citizen.currentTile = citizen.commutePath[citizen.pathIndex];
            ++citizen.pathIndex;
        }

        if (citizen.pathIndex >= pathSize)
        {
            citizen.commutingToWork = false;
            citizen.movementProgress = 0.0f;
        }
        else if (simulationDeltaTime > 0.0f)
        {
            // En route and time is flowing: actually moving.
            // While paused the count reads zero although commuters
            // hold their positions mid-road.
            ++movingCitizens;
        }
    }
}

int CitizenMovement::getMovingCitizens() const
{
    return movingCitizens;
}

}  // namespace urbania
