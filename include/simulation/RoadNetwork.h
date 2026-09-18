#pragma once

#include <map>
#include <utility>
#include <vector>

#include "world/Tile.h"

class World;

namespace urbania {

// Road graph for future pathfinding, traffic, and transport systems.
// Nodes are Road tiles keyed by coordinate; edges link the four
// cardinal neighbors (no diagonals). Only Road tiles are ever nodes.
//
// The graph is rebuilt on demand via rebuild() (after road construction
// or demolition), never every frame. Ordered containers keep iteration
// deterministic. No raylib dependency here.
class RoadNetwork {
public:
    void rebuild(const World& world);
    void clear();

    bool isRoad(int x, int y) const;
    std::vector<TileCoordinate> getNeighbors(const TileCoordinate& tile) const;
    int getNodeCount() const;
    bool areConnected(int x1, int y1, int x2, int y2) const;

private:
    std::map<std::pair<int, int>, std::vector<TileCoordinate>> adjacency;
};

}  // namespace urbania
