#include "simulation/Pollution.h"

#include <algorithm>
#include <vector>

#include "world/World.h"

namespace urbania {

Pollution::Pollution()
{
}

void Pollution::update(const World& world, float simulationDeltaTime)
{
    // Zero or negative delta (e.g. paused) halts simulation progression.
    if (simulationDeltaTime <= 0.0f)
    {
        return;
    }

    secondsTowardNextHour += simulationDeltaTime;
    while (secondsTowardNextHour >= SIM_SECONDS_PER_HOUR)
    {
        secondsTowardNextHour -= SIM_SECONDS_PER_HOUR;
        step(world);
    }
}

void Pollution::step(const World& world)
{
    const int width = world.getWidth();
    const int height = world.getHeight();
    if (width <= 0 || height <= 0)
    {
        return;
    }

    // Intermediate 2D buffer for generation and natural decay.
    std::vector<std::vector<float>> intermediate(height, std::vector<float>(width, 0.0f));

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float p = getPollution(x, y);

            // 1. Industrial pollution emission
            if (world.getTile(x, y).type == TileType::Industrial)
            {
                p += INDUSTRIAL_POLLUTION_PER_HOUR;
            }

            // 2. Natural decay
            if (p > 0.0f)
            {
                p = std::max(MIN_POLLUTION, p - NATURAL_POLLUTION_DECAY_PER_HOUR);
            }

            intermediate[y][x] = p;
        }
    }

    // Buffer for 4-cardinal diffusion (Up, Right, Down, Left).
    std::vector<std::vector<float>> diffused(height, std::vector<float>(width, 0.0f));
    constexpr int DX[4] = { 0, 1, 0, -1 };
    constexpr int DY[4] = { -1, 0, 1, 0 };

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const float p = intermediate[y][x];
            if (p <= 0.0f)
            {
                continue;
            }

            // Count in-bounds cardinal neighbors.
            int validNeighbors = 0;
            for (int i = 0; i < 4; ++i)
            {
                const int nx = x + DX[i];
                const int ny = y + DY[i];
                if (nx >= 0 && nx < width && ny >= 0 && ny < height)
                {
                    ++validNeighbors;
                }
            }

            if (validNeighbors > 0)
            {
                const float diffuseAmount = p * DIFFUSION_RATE;
                diffused[y][x] += (p - diffuseAmount);
                const float perNeighbor = diffuseAmount / static_cast<float>(validNeighbors);
                for (int i = 0; i < 4; ++i)
                {
                    const int nx = x + DX[i];
                    const int ny = y + DY[i];
                    if (nx >= 0 && nx < width && ny >= 0 && ny < height)
                    {
                        diffused[ny][nx] += perNeighbor;
                    }
                }
            }
            else
            {
                diffused[y][x] += p;
            }
        }
    }

    // 3. Park pollution reduction in park tile and adjacent cardinal tiles
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            if (world.getTile(x, y).type == TileType::Park)
            {
                diffused[y][x] =
                    std::max(MIN_POLLUTION, diffused[y][x] - PARK_POLLUTION_REDUCTION_PER_HOUR);
                for (int i = 0; i < 4; ++i)
                {
                    const int nx = x + DX[i];
                    const int ny = y + DY[i];
                    if (nx >= 0 && nx < width && ny >= 0 && ny < height)
                    {
                        diffused[ny][nx] =
                            std::max(MIN_POLLUTION,
                                     diffused[ny][nx] - PARK_POLLUTION_REDUCTION_PER_HOUR);
                    }
                }
            }
        }
    }

    // 4. Clamping and storage in the coordinate-keyed map
    grid.clear();
    float total = 0.0f;
    maxPollution = 0.0f;

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const float finalP = std::clamp(diffused[y][x], MIN_POLLUTION, MAX_POLLUTION);
            if (finalP > 0.001f)
            {
                grid[{ x, y, true }] = finalP;
                total += finalP;
                if (finalP > maxPollution)
                {
                    maxPollution = finalP;
                }
            }
        }
    }

    averagePollution = total / static_cast<float>(width * height);
}

float Pollution::getPollution(int x, int y) const
{
    const auto it = grid.find({ x, y, true });
    if (it != grid.end())
    {
        return it->second;
    }
    return 0.0f;
}

float Pollution::getPollution(const TileCoordinate& coord) const
{
    if (!coord.valid)
    {
        return 0.0f;
    }
    return getPollution(coord.x, coord.y);
}

float Pollution::getAveragePollution() const
{
    return averagePollution;
}

float Pollution::getMaxPollution() const
{
    return maxPollution;
}

const std::map<TileCoordinate, float>& Pollution::getPollutionGrid() const
{
    return grid;
}

}  // namespace urbania
