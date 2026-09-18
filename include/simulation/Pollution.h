#pragma once

#include <map>

#include "world/Tile.h"

class World;

namespace urbania {

// City pollution simulation.
// Tracks per-tile pollution in the range [0.0f, 100.0f].
//
// 0.0f   = clean
// 100.0f = very polluted
//
// - Industrial tiles generate pollution (+10/hr).
// - Natural decay slowly dissipates pollution (-1/hr).
// - Pollution diffuses to the four cardinal neighbors (Up, Right, Down, Left).
// - Parks reduce pollution in their tile and cardinal adjacent tiles (-4/hr).
// - Updates advance once per simulation hour based on simulation time.
// - Stored in a separate data structure keyed by TileCoordinate, not in Tile.
class Pollution {
public:
    static constexpr float MIN_POLLUTION = 0.0f;
    static constexpr float MAX_POLLUTION = 100.0f;

    static constexpr float INDUSTRIAL_POLLUTION_PER_HOUR = 10.0f;
    static constexpr float PARK_POLLUTION_REDUCTION_PER_HOUR = 4.0f;
    static constexpr float NATURAL_POLLUTION_DECAY_PER_HOUR = 1.0f;
    static constexpr float DIFFUSION_RATE = 0.10f;

    static constexpr float SIM_SECONDS_PER_HOUR = 3600.0f;

    Pollution();

    // Advances the pollution timer on simulation time. Call once per frame
    // with the scaled simulation delta (0 while paused).
    void update(const World& world, float simulationDeltaTime);

    // Forces an immediate single-step execution of the pollution diffusion cycle.
    void step(const World& world);

    float getPollution(int x, int y) const;
    float getPollution(const TileCoordinate& coord) const;
    float getAveragePollution() const;
    float getMaxPollution() const;
    const std::map<TileCoordinate, float>& getPollutionGrid() const;

private:
    std::map<TileCoordinate, float> grid;
    float secondsTowardNextHour = 0.0f;
    float averagePollution = 0.0f;
    float maxPollution = 0.0f;
};

}  // namespace urbania
