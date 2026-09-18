#include "simulation/Simulation.h"

#include "world/World.h"

bool Simulation::initialize(World& world)
{
    this->world = &world;
    elapsedSimulationSeconds = 0.0f;
    return true;
}

void Simulation::update(float simulationDeltaTime)
{
    if (world == nullptr)
    {
        return;
    }

    elapsedSimulationSeconds += simulationDeltaTime;

    // Future city systems run here on simulation time:
    // population.update(simulationDeltaTime);
    // economy.update(simulationDeltaTime);
    // traffic.update(simulationDeltaTime);
    // pollution.update(simulationDeltaTime);
    // happiness.update(simulationDeltaTime);
}

void Simulation::shutdown()
{
    world = nullptr;
}

float Simulation::getElapsedSimulationSeconds() const
{
    return elapsedSimulationSeconds;
}
