#include "simulation/Simulation.h"

#include "world/World.h"

bool Simulation::initialize(World& world)
{
    this->world = &world;
    elapsedSimulationSeconds = 0.0f;
    population = Population();
    employment = urbania::Employment();
    roadNetwork = urbania::RoadNetwork();
    roadNetwork.rebuild(world);
    commuteSystem = urbania::CommuteSystem();
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
    commuteSystem.update(*world, roadNetwork, population.getCitizenManager());
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

urbania::RoadNetwork& Simulation::getRoadNetwork()
{
    return roadNetwork;
}

const urbania::CommuteSystem& Simulation::getCommuteSystem() const
{
    return commuteSystem;
}
