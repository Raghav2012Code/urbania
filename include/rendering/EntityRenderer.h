#pragma once

#include <vector>

#include "raylib.h"
#include "rendering/TextureManager.h"
#include "simulation/Bus.h"
#include "simulation/Citizen.h"
#include "simulation/Vehicle.h"

namespace urbania {

class CitizenManager;
class Traffic;
class Transit;

// Renders dynamic world entities: citizens/pedestrians, private cars, and transit buses.
// Scales and positions correctly inside the 2D world camera.
class EntityRenderer {
public:
    EntityRenderer();

    // Renders all citizens
    void renderCitizens(const CitizenManager& citizens, int tileSize);

    // Renders private traffic vehicles
    void renderVehicles(const Traffic& traffic, int tileSize);

    // Renders transit bus vehicles
    void renderBuses(const Transit& transit, int tileSize);

private:
    void drawPedestrian(float x, float y, int citizenId, float progress);
    void drawCar(float x, float y, float headingRad, int vehicleId);
    void drawBus(float x, float y, float headingRad, int busId, int routeId);
};

}  // namespace urbania
