#pragma once

namespace urbania {

class CitizenManager;

// Moves employed citizens along their stored commute paths: home to
// first road tile, then road by road, stopping at the road tile
// adjacent to the workplace. Progress advances in simulation time,
// so pause and speed controls apply automatically.
//
// Leg model: pathIndex is the current leg (0 = home to path[0],
// k = path[k-1] to path[k]); movementProgress runs 0.0 to 1.0 within
// the leg. When pathIndex reaches the path size, the citizen arrived.
// Unemployed or routeless citizens reset to home and stay put.
// No raylib dependency here; rendering interpolates separately.
class CitizenMovement {
public:
    static constexpr float TILES_PER_SIM_SECOND = 2.0f;

    void update(CitizenManager& citizens, float simulationDeltaTime);

    int getMovingCitizens() const;

private:
    int movingCitizens = 0;
};

}  // namespace urbania
