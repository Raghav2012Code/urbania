#pragma once

#include <cstdint>
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
//
// revision() counts rebuilds. Node count alone cannot describe the graph:
// demolishing one road and building another leaves the count unchanged
// while connectivity differs completely. Consumers that cache derived
// results (CommuteSystem) must key on this instead.
class RoadNetwork {
public:
    void rebuild(const World& world);
    void clear();

    bool isRoad(int x, int y) const;
    std::vector<TileCoordinate> getNeighbors(const TileCoordinate& tile) const;
    int getNodeCount() const;
    bool areConnected(int x1, int y1, int x2, int y2) const;

    // Monotonic rebuild counter. Unsigned 64-bit so it cannot overflow
    // into a value that would alias an earlier revision.
    std::uint64_t revision() const;

private:
    std::map<std::pair<int, int>, std::vector<TileCoordinate>> adjacency;
    std::uint64_t graphRevision = 0;
};

}  // namespace urbania
