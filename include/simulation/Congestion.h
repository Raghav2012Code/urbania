#pragma once

#include <map>
#include <utility>

namespace urbania {

class Traffic;

// Per-road-tile traffic load. Counts active vehicles by the road tile
// each currently occupies and exposes normalized congestion:
// 0.0 = empty, 1.0 = at capacity, above = overloaded (never clamped
// internally so overload stays visible).
//
// Rebuilt from scratch on every update, so demolished roads leave no
// stale entries behind. Also owns the congestion-to-speed formula used
// by Traffic. No raylib dependency here.
class Congestion {
public:
    static constexpr int ROAD_CAPACITY = 5;

    void update(const Traffic& traffic);

    static int getCapacity();
    int getVehicleCount(int x, int y) const;
    float getCongestion(int x, int y) const;
    int getCongestedRoadCount() const;
    float getMaxCongestion() const;

    static float speedMultiplier(float congestion);

private:
    std::map<std::pair<int, int>, int> usage;
    int congestedRoadCount = 0;
    float maxCongestion = 0.0f;
};

}  // namespace urbania
