#include "simulation/Pathfinder.h"

#include <cstdint>
#include <map>
#include <queue>
#include <utility>
#include <vector>

namespace urbania {
namespace {

using Coord = std::pair<int, int>;

int manhattan(const Coord& a, const Coord& b)
{
    const int dx = a.first > b.first ? a.first - b.first : b.first - a.first;
    const int dy = a.second > b.second ? a.second - b.second : b.second - a.second;
    return dx + dy;
}

struct OpenNode {
    int fScore;
    std::uint64_t order;
    Coord coord;
};

struct OpenNodeCompare {
    bool operator()(const OpenNode& a, const OpenNode& b) const
    {
        // Min-heap on f-score; insertion order breaks ties so equal
        // costs always expand deterministically.
        if (a.fScore != b.fScore)
        {
            return a.fScore > b.fScore;
        }
        return a.order > b.order;
    }
};

// Expansion order: Up, Right, Down, Left.
constexpr int DX[4] = { 0, 1, 0, -1 };
constexpr int DY[4] = { -1, 0, 1, 0 };

}  // namespace

std::vector<TileCoordinate> Pathfinder::findPath(const RoadNetwork& roadNetwork,
                                                 TileCoordinate start, TileCoordinate goal)
{
    if (!start.valid || !goal.valid)
    {
        return {};
    }
    if (!roadNetwork.isRoad(start.x, start.y) || !roadNetwork.isRoad(goal.x, goal.y))
    {
        return {};
    }

    const Coord startCoord{ start.x, start.y };
    const Coord goalCoord{ goal.x, goal.y };

    if (startCoord == goalCoord)
    {
        return { start };
    }

    std::priority_queue<OpenNode, std::vector<OpenNode>, OpenNodeCompare> open;
    std::map<Coord, int> gScore;
    std::map<Coord, Coord> cameFrom;
    std::map<Coord, bool> closed;

    std::uint64_t order = 0;
    gScore[startCoord] = 0;
    open.push({ manhattan(startCoord, goalCoord), order++, startCoord });

    while (!open.empty())
    {
        const Coord current = open.top().coord;
        open.pop();

        if (closed[current])
        {
            continue;
        }
        closed[current] = true;

        if (current == goalCoord)
        {
            std::vector<TileCoordinate> path;
            Coord step = goalCoord;
            while (!(step == startCoord))
            {
                path.push_back({ step.first, step.second, true });
                step = cameFrom[step];
            }
            path.push_back(start);

            std::vector<TileCoordinate> ordered;
            ordered.reserve(path.size());
            for (auto it = path.rbegin(); it != path.rend(); ++it)
            {
                ordered.push_back(*it);
            }
            return ordered;
        }

        for (int i = 0; i < 4; ++i)
        {
            const int nx = current.first + DX[i];
            const int ny = current.second + DY[i];

            if (!roadNetwork.areConnected(current.first, current.second, nx, ny))
            {
                continue;
            }

            const Coord neighbor{ nx, ny };
            if (closed[neighbor])
            {
                continue;
            }

            const int tentative = gScore[current] + 1;
            const auto existing = gScore.find(neighbor);
            if (existing != gScore.end() && tentative >= existing->second)
            {
                continue;
            }

            cameFrom[neighbor] = current;
            gScore[neighbor] = tentative;
            open.push({ tentative + manhattan(neighbor, goalCoord), order++, neighbor });
        }
    }

    return {};
}

}  // namespace urbania
