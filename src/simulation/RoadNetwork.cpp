#include "simulation/RoadNetwork.h"

#include "world/Tile.h"
#include "world/World.h"

namespace urbania {

void RoadNetwork::rebuild(const World& world)
{
    std::map<std::pair<int, int>, std::vector<TileCoordinate>> fresh;

    for (int y = 0; y < world.getHeight(); ++y)
    {
        for (int x = 0; x < world.getWidth(); ++x)
        {
            if (world.getTile(x, y).type != TileType::Road)
            {
                continue;
            }

            // Four cardinal neighbors only, in fixed Up/Down/Left/Right
            // order so iteration stays deterministic.
            static constexpr int DX[4] = { 0, 0, -1, 1 };
            static constexpr int DY[4] = { -1, 1, 0, 0 };

            std::vector<TileCoordinate> neighbors;
            for (int i = 0; i < 4; ++i)
            {
                const int nx = x + DX[i];
                const int ny = y + DY[i];
                if (nx < 0 || nx >= world.getWidth() || ny < 0 || ny >= world.getHeight())
                {
                    continue;
                }
                if (world.getTile(nx, ny).type == TileType::Road)
                {
                    neighbors.push_back({ nx, ny, true });
                }
            }

            fresh[{ x, y }] = neighbors;
        }
    }

    adjacency = fresh;
}

void RoadNetwork::clear()
{
    adjacency.clear();
}

bool RoadNetwork::isRoad(int x, int y) const
{
    return adjacency.find({ x, y }) != adjacency.end();
}

std::vector<TileCoordinate> RoadNetwork::getNeighbors(const TileCoordinate& tile) const
{
    if (!tile.valid)
    {
        return {};
    }

    const auto it = adjacency.find({ tile.x, tile.y });
    if (it == adjacency.end())
    {
        return {};
    }
    return it->second;
}

int RoadNetwork::getNodeCount() const
{
    return static_cast<int>(adjacency.size());
}

bool RoadNetwork::areConnected(int x1, int y1, int x2, int y2) const
{
    // Connected means both are road nodes exactly one cardinal step apart.
    const int distance = (x1 > x2 ? x1 - x2 : x2 - x1) + (y1 > y2 ? y1 - y2 : y2 - y1);
    if (distance != 1)
    {
        return false;
    }

    const auto it = adjacency.find({ x1, y1 });
    if (it == adjacency.end())
    {
        return false;
    }

    for (const TileCoordinate& neighbor : it->second)
    {
        if (neighbor.x == x2 && neighbor.y == y2)
        {
            return true;
        }
    }
    return false;
}

}  // namespace urbania
