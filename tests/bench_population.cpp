#include <chrono>
#include <cstdlib>
#include <iostream>

#include "world/World.h"
#include "world/Tile.h"
#include "simulation/Population.h"

using namespace urbania;

// Non-CTest benchmark for the population growth loop. Reports wall-clock cost
// of Population::update at a small and a large number of homes so the O(homes *
// citizens) regression cannot silently return. Not timing-gated (hardware
// dependent); run manually:
//
//   ./build/bench_population.exe [homes] [hours]
namespace {

double runCase(int homes, int hours)
{
    World world;

    // Lay homes out row-major, leaving odd rows free (mirrors a zoned city).
    int placed = 0;
    for (int y = 0; y < world.getHeight() && placed < homes; ++y)
    {
        for (int x = 0; x < world.getWidth() && placed < homes; ++x)
        {
            world.getTile(x, y).type = TileType::Residential;
            ++placed;
        }
    }

    Population population;
    population.update(world, 0.0f);

    const auto start = std::chrono::steady_clock::now();
    for (int hour = 0; hour < hours; ++hour)
    {
        population.update(world, 3600.0f);
    }
    const auto end = std::chrono::steady_clock::now();

    // Sanity: the flat resident cache must sum to the live population.
    int summed = 0;
    for (int y = 0; y < world.getHeight(); ++y)
    {
        for (int x = 0; x < world.getWidth(); ++x)
        {
            summed += population.getResidentsAt(x, y);
        }
    }
    if (summed != population.getTotalPopulation())
    {
        std::cerr << "INVARIANT FAILED: summed=" << summed
                  << " total=" << population.getTotalPopulation() << "\n";
        std::exit(2);
    }

    const double ms =
        std::chrono::duration<double, std::milli>(end - start).count();
    std::cout << "homes=" << placed << " hours=" << hours
              << " total_ms=" << ms << " ms_per_update=" << (ms / hours)
              << " final_pop=" << population.getTotalPopulation() << "\n";
    return ms / hours;
}

}  // namespace

int main(int argc, char** argv)
{
    int homes = 300;
    int hours = 30;
    if (argc > 1)
    {
        homes = std::atoi(argv[1]);
    }
    if (argc > 2)
    {
        hours = std::atoi(argv[2]);
    }

    std::cout << "=== Population growth benchmark ===\n";
    const double small = runCase(300, hours);
    const double large = runCase(homes, hours);

    // Linear scaling: 10x the homes must stay well under 10x the time.
    std::cout << "small_ms_per_update=" << small << " large_ms_per_update=" << large
              << " ratio=" << (small > 0.0 ? large / small : 0.0) << "\n";
    return 0;
}
