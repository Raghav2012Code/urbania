#pragma once

#include "simulation/CitizenMovement.h"
#include "simulation/CommuteSystem.h"
#include "simulation/Congestion.h"
#include "simulation/Demand.h"
#include "simulation/Economy.h"
#include "simulation/Employment.h"
#include "simulation/Population.h"
#include "simulation/RoadNetwork.h"
#include "simulation/Traffic.h"

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
    const urbania::Employment& getEmployment() const;
    urbania::RoadNetwork& getRoadNetwork();
    const urbania::CommuteSystem& getCommuteSystem() const;
    const urbania::CitizenMovement& getCitizenMovement() const;
    const urbania::Traffic& getTraffic() const;
    const urbania::Congestion& getCongestion() const;
    const Economy& getEconomy() const;
    Economy& getEconomy();
    const Demand& getDemand() const;
    Demand& getDemand();

private:
    World* world = nullptr;
    Population population;
    urbania::Employment employment;
    urbania::RoadNetwork roadNetwork;
    urbania::CommuteSystem commuteSystem;
    urbania::CitizenMovement citizenMovement;
    urbania::Traffic traffic;
    urbania::Congestion congestion;
    Economy economy;
    Demand demand;

    // Development-only counter proving the pipeline runs on
    // simulation time. Not a gameplay mechanic.
    float elapsedSimulationSeconds = 0.0f;
};

