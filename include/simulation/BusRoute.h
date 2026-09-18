#pragma once

#include <vector>

namespace urbania {

// Represents an ordered sequence of bus stop IDs forming a transit route.
// IDs are deterministic and start at 1.
struct BusRoute {
    int id = 0;
    std::vector<int> stopIds;
};

}  // namespace urbania
