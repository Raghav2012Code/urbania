#pragma once

#include <map>
#include <vector>

#include "world/Tile.h"

class World;

namespace urbania {

struct TileUtilityStatus {
    bool connected = false;
    bool hasElectricity = false;
    bool hasWater = false;
    bool hasSewage = false;
    bool isFullySupplied = false;
};

// Basic City Utilities System: Electricity, Water, and Sewage.
//
// Coverage rule:
// - A developed tile (Residential, Commercial, Industrial) is connected to
//   utilities if there is a Road tile within UTILITY_CONNECTION_RADIUS (2 tiles).
//
// Demand per developed tile:
// - Residential: Electricity 1, Water 1, Sewage 1
// - Commercial:  Electricity 2, Water 2, Sewage 2
// - Industrial:  Electricity 4, Water 3, Sewage 3
// - Others:      Electricity 0, Water 0, Sewage 0
//
// Capacity:
// - Electricity: 100
// - Water:       100
// - Sewage:      100
//
// Deterministic allocation:
// - Buildings are processed in stable coordinate order (row-major).
// - If demand exceeds capacity, remaining buildings become unsupplied.
//
// Maintenance:
// - Electricity: ₹500/day
// - Water:       ₹300/day
// - Sewage:      ₹300/day
// - Total:       ₹1,100/day
class Utilities {
public:
    static constexpr int UTILITY_CONNECTION_RADIUS = 2;

    static constexpr int DEFAULT_ELECTRICITY_CAPACITY = 100;
    static constexpr int DEFAULT_WATER_CAPACITY = 100;
    static constexpr int DEFAULT_SEWAGE_CAPACITY = 100;

    static constexpr float ELECTRICITY_DAILY_MAINTENANCE = 500.0f;
    static constexpr float WATER_DAILY_MAINTENANCE = 300.0f;
    static constexpr float SEWAGE_DAILY_MAINTENANCE = 300.0f;
    static constexpr float TOTAL_DAILY_MAINTENANCE = 1100.0f;

    static constexpr int RESIDENTIAL_ELECTRICITY_DEMAND = 1;
    static constexpr int RESIDENTIAL_WATER_DEMAND = 1;
    static constexpr int RESIDENTIAL_SEWAGE_DEMAND = 1;

    static constexpr int COMMERCIAL_ELECTRICITY_DEMAND = 2;
    static constexpr int COMMERCIAL_WATER_DEMAND = 2;
    static constexpr int COMMERCIAL_SEWAGE_DEMAND = 2;

    static constexpr int INDUSTRIAL_ELECTRICITY_DEMAND = 4;
    static constexpr int INDUSTRIAL_WATER_DEMAND = 3;
    static constexpr int INDUSTRIAL_SEWAGE_DEMAND = 3;

    Utilities();

    void update(const World& world, float simulationDeltaTime);
    void recalculate(const World& world);

    // Queries
    bool isConnectedToRoad(const World& world, int x, int y) const;
    bool isConnectedToRoad(const World& world, const TileCoordinate& coord) const;

    TileUtilityStatus getTileStatus(int x, int y) const;
    TileUtilityStatus getTileStatus(const TileCoordinate& coord) const;
    bool isTileSupplied(int x, int y) const;
    bool isTileSupplied(const TileCoordinate& coord) const;

    int getElectricityDemand() const;
    int getWaterDemand() const;
    int getSewageDemand() const;

    int getElectricityCapacity() const;
    int getWaterCapacity() const;
    int getSewageCapacity() const;

    int getSuppliedBuildingCount() const;
    int getUnsuppliedBuildingCount() const;
    int getTotalDevelopedBuildingCount() const;

    float getDailyMaintenanceCost() const;

    const std::map<TileCoordinate, TileUtilityStatus>& getStatusGrid() const;

    static void getDemandsForType(TileType type, int& outElec, int& outWater, int& outSewage);

private:
    int electricityCapacity = DEFAULT_ELECTRICITY_CAPACITY;
    int waterCapacity = DEFAULT_WATER_CAPACITY;
    int sewageCapacity = DEFAULT_SEWAGE_CAPACITY;

    int electricityDemand = 0;
    int waterDemand = 0;
    int sewageDemand = 0;

    int suppliedBuildings = 0;
    int unsuppliedBuildings = 0;
    int totalDevelopedBuildings = 0;

    std::map<TileCoordinate, TileUtilityStatus> statusGrid;
};

}  // namespace urbania
