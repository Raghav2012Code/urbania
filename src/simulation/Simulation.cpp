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
    citizenMovement = urbania::CitizenMovement();
    traffic = urbania::Traffic();
    congestion = urbania::Congestion();
    economy = Economy();
    demand = Demand();
    demand.recalculate(world, population, employment);
    pollution = urbania::Pollution();
    happiness = urbania::Happiness();
    happiness.recalculate(world, population.getCitizenManager(), pollution);
    landValue = urbania::LandValue();
    landValue.recalculate(world, pollution, congestion);
    housing = urbania::Housing();
    housing.recalculate(world, population, landValue);
    transit = urbania::Transit();
    transit.syncWithWorld(world);
    return true;
}

void Simulation::update(float simulationDeltaTime)
{
    if (world == nullptr)
    {
        return;
    }

    elapsedSimulationSeconds += simulationDeltaTime;

    const int popBefore = population.getTotalPopulation();
    const int empBefore = employment.getEmployedCitizens();

    population.update(*world, simulationDeltaTime);
    employment.update(*world, population.getCitizenManager(), simulationDeltaTime);
    commuteSystem.update(*world, roadNetwork, population.getCitizenManager());
    citizenMovement.update(population.getCitizenManager(), simulationDeltaTime);
    // Traffic moves on last update's congestion, then congestion is
    // recalculated from the new positions: one clean pass, no
    // within-frame feedback loop.
    traffic.update(roadNetwork, population.getCitizenManager(), simulationDeltaTime, congestion);
    congestion.update(traffic);
    economy.update(*world, population, employment, simulationDeltaTime);
    demand.update(*world, population, employment, simulationDeltaTime);
    pollution.update(*world, simulationDeltaTime);
    happiness.update(*world, population.getCitizenManager(), pollution, simulationDeltaTime);
    landValue.update(*world, pollution, congestion, simulationDeltaTime);
    housing.update(*world, population, landValue, simulationDeltaTime);
    transit.syncWithWorld(*world);
    transit.update(simulationDeltaTime, roadNetwork);

    if (population.getTotalPopulation() != popBefore ||
        employment.getEmployedCitizens() != empBefore)
    {
        demand.recalculate(*world, population, employment);
        housing.recalculate(*world, population, landValue);
        happiness.recalculate(*world, population.getCitizenManager(), pollution);
    }
}

void Simulation::onWorldModified()
{
    if (world == nullptr)
    {
        return;
    }

    roadNetwork.rebuild(*world);
    population.update(*world, 0.0f);
    employment.update(*world, population.getCitizenManager(), 0.0f);
    commuteSystem.update(*world, roadNetwork, population.getCitizenManager());
    transit.syncWithWorld(*world);
    demand.recalculate(*world, population, employment);
    landValue.recalculate(*world, pollution, congestion);
    housing.recalculate(*world, population, landValue);
    happiness.recalculate(*world, population.getCitizenManager(), pollution);
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

const urbania::RoadNetwork& Simulation::getRoadNetwork() const
{
    return roadNetwork;
}

urbania::RoadNetwork& Simulation::getRoadNetwork()
{
    return roadNetwork;
}

const urbania::CommuteSystem& Simulation::getCommuteSystem() const
{
    return commuteSystem;
}

const urbania::CitizenMovement& Simulation::getCitizenMovement() const
{
    return citizenMovement;
}

const urbania::Traffic& Simulation::getTraffic() const
{
    return traffic;
}

const urbania::Congestion& Simulation::getCongestion() const
{
    return congestion;
}

const Economy& Simulation::getEconomy() const
{
    return economy;
}

Economy& Simulation::getEconomy()
{
    return economy;
}

const Demand& Simulation::getDemand() const
{
    return demand;
}

Demand& Simulation::getDemand()
{
    return demand;
}

const urbania::Pollution& Simulation::getPollution() const
{
    return pollution;
}

urbania::Pollution& Simulation::getPollution()
{
    return pollution;
}

const urbania::Happiness& Simulation::getHappiness() const
{
    return happiness;
}

urbania::Happiness& Simulation::getHappiness()
{
    return happiness;
}

const urbania::LandValue& Simulation::getLandValue() const
{
    return landValue;
}

urbania::LandValue& Simulation::getLandValue()
{
    return landValue;
}

const urbania::Housing& Simulation::getHousing() const
{
    return housing;
}

urbania::Housing& Simulation::getHousing()
{
    return housing;
}

const urbania::Transit& Simulation::getTransit() const
{
    return transit;
}

urbania::Transit& Simulation::getTransit()
{
    return transit;
}




