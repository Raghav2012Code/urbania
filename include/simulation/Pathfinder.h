#pragma once

#include <vector>

#include "simulation/RoadNetwork.h"
#include "world/Tile.h"

namespace urbania {

// Standard A* over the RoadNetwork. Rules: four cardinal moves only
// (Up, Right, Down, Left expansion order), road tiles only, uniform
// cost 1 per step, Manhattan-distance heuristic. Uses nothing but the
// graph, so Grass and buildings are never traversed.
//
// Returns the full tile path including start and goal, or an empty
// vector when no route exists. Deterministic: same graph + endpoints
// always yield the same path. No raylib dependency here.
class Pathfinder {
public:
    static std::vector<TileCoordinate> findPath(const RoadNetwork& roadNetwork,
                                                TileCoordinate start, TileCoordinate goal);
};

}  // namespace urbania
