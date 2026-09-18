#pragma once

#include "simulation/Population.h"

class World;

// City simulation manager. Receives scaled simulation time from the
// SimulationClock (never raw frame time) and will eventually coordinate
// population, economy, traffic, pollution, and happiness systems.
//
// Simulation only mutates World state; rendering reads World state
// separately. No raylib dependency here.
class Simulation {
public:
    bool initialize(World& world);
    void update(float simulationDeltaTime);
    void shutdown();

    float getElapsedSimulationSeconds() const;
    const Population& getPopulation() const;

private:
    World* world = nullptr;
    Population population;

    // Development-only counter proving the pipeline runs on
    // simulation time. Not a gameplay mechanic.
    float elapsedSimulationSeconds = 0.0f;
};
