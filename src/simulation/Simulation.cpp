#include "simulation/Simulation.h"

#include "world/World.h"

bool Simulation::initialize(World& world)
{
    this->world = &world;
    elapsedSimulationSeconds = 0.0f;
    population = Population();
    employment = urbania::Employment();
    return true;
}

void Simulation::update(float simulationDeltaTime)
{
    if (world == nullptr)
    {
        return;
    }

    elapsedSimulationSeconds += simulationDeltaTime;

    population.update(*world, simulationDeltaTime);
    employment.update(*world, population.getCitizenManager(), simulationDeltaTime);
    // Future city systems run here on simulation time:
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

const Population& Simulation::getPopulation() const
{
    return population;
}

const urbania::Employment& Simulation::getEmployment() const
{
    return employment;
}
