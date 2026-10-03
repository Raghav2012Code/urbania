#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "world/World.h"
#include "world/Tile.h"
#include "simulation/Simulation.h"
#include "simulation/Population.h"

using namespace urbania;

// Non-CTest benchmark for #5: drives Simulation::update on a dense, developed
// 80x80 city at an 8x-speed frame delta and reports average/worst frame cost.
// Not timing-gated; run manually:
//
//   ./build/bench_simulation.exe [frames] [warmup_hours]
namespace {

// Roads on every odd row; the even rows are zoned. By default they mix
// residential / commercial / industrial so jobs and commuters exist; with
// residentialHeavy they are all residential to stress the population path.
void buildDenseCity(World& world, bool residentialHeavy)
{
    for (int y = 0; y < world.getHeight(); ++y)
    {
        for (int x = 0; x < world.getWidth(); ++x)
        {
            if (y % 2 == 1)
            {
                world.getTile(x, y).type = TileType::Road;
            }
            else if (residentialHeavy)
            {
                world.getTile(x, y).type = TileType::Residential;
            }
            else if (x % 3 == 0)
            {
                world.getTile(x, y).type = TileType::Commercial;
            }
            else if (x % 3 == 1)
            {
                world.getTile(x, y).type = TileType::Industrial;
            }
            else
            {
                world.getTile(x, y).type = TileType::Residential;
            }
        }
    }
}

}  // namespace

int main(int argc, char** argv)
{
    int frames = 300;
    int warmupHours = 12;
    bool residentialHeavy = false;
    if (argc > 1)
    {
        frames = std::atoi(argv[1]);
    }
    if (argc > 2)
    {
        warmupHours = std::atoi(argv[2]);
    }
    if (argc > 3)
    {
        residentialHeavy = std::atoi(argv[3]) != 0;
    }

    World world;
    buildDenseCity(world, residentialHeavy);

    Simulation simulation;
    simulation.initialize(world);

    for (int hour = 0; hour < warmupHours; ++hour)
    {
        simulation.update(3600.0f);
    }

    // 8x speed: one 60 FPS frame advances the sim by (1/60)*8*60 = 8 seconds.
    const float frameDelta = 8.0f;
    std::vector<double> samples;
    samples.reserve(static_cast<std::size_t>(frames));

    for (int frame = 0; frame < frames; ++frame)
    {
        const auto start = std::chrono::steady_clock::now();
        simulation.update(frameDelta);
        const auto end = std::chrono::steady_clock::now();
        samples.push_back(
            std::chrono::duration<double, std::milli>(end - start).count());
    }

    double total = 0.0;
    for (double sample : samples)
    {
        total += sample;
    }
    const double average = total / static_cast<double>(frames);
    const double worst = *std::max_element(samples.begin(), samples.end());

    std::cout << "pop=" << simulation.getPopulation().getTotalPopulation()
              << " jobs=" << simulation.getEmployment().getTotalJobs()
              << " nodes=" << simulation.getRoadNetwork().getNodeCount() << "\n";
    std::cout << "frames=" << frames << " avg_ms=" << average << " worst_ms=" << worst
              << " budget_ms=16.67\n";
    return 0;
}
