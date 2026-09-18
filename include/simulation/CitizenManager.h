#pragma once

#include <vector>

#include "simulation/Citizen.h"

namespace urbania {

// Owns all Citizen entities: creation, removal, lookup, and ID tracking.
// Simple vector storage, linear scans. IDs start at 1 and increment
// forever; removed IDs are never reused, so references stay stable.
// Deterministic: creation order fully determines IDs.
class CitizenManager {
public:
    static constexpr float DEFAULT_INCOME = 30000.0f;
    static constexpr float DEFAULT_HAPPINESS = 50.0f;

    Citizen& createCitizen(const TileCoordinate& home);
    void removeCitizen(int id);
    void removeCitizensAt(const TileCoordinate& home);
    void clear();

    Citizen* getCitizen(int id);
    const Citizen* getCitizen(int id) const;
    const std::vector<Citizen>& getCitizens() const;
    int getCitizenCount() const;

private:
    std::vector<Citizen> citizens;
    int nextId = 1;
};

}  // namespace urbania
