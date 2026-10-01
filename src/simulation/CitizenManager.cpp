#include "simulation/CitizenManager.h"

#include <algorithm>
#include <limits>

namespace urbania {

Citizen& CitizenManager::createCitizen(const TileCoordinate& home)
{
    Citizen citizen;
    citizen.id = nextId;
    citizen.home = home;
    citizen.currentTile = home;
    citizen.income = DEFAULT_INCOME;
    citizen.happiness = DEFAULT_HAPPINESS;
    // employed stays false, workplace stays invalid until jobs exist.
    // Movement starts at home once a commute route arrives.
    ++nextId;

    citizens.push_back(citizen);
    return citizens.back();
}

void CitizenManager::removeCitizen(int id)
{
    for (auto it = citizens.begin(); it != citizens.end();)
    {
        if (it->id == id)
        {
            it = citizens.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void CitizenManager::removeCitizensAt(const TileCoordinate& home)
{
    for (auto it = citizens.begin(); it != citizens.end();)
    {
        if (it->home.valid && it->home.x == home.x && it->home.y == home.y)
        {
            it = citizens.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void CitizenManager::clear()
{
    citizens.clear();
}

Citizen* CitizenManager::getCitizen(int id)
{
    for (Citizen& citizen : citizens)
    {
        if (citizen.id == id)
        {
            return &citizen;
        }
    }
    return nullptr;
}

const Citizen* CitizenManager::getCitizen(int id) const
{
    for (const Citizen& citizen : citizens)
    {
        if (citizen.id == id)
        {
            return &citizen;
        }
    }
    return nullptr;
}

const std::vector<Citizen>& CitizenManager::getCitizens() const
{
    return citizens;
}

std::vector<Citizen>& CitizenManager::getCitizens()
{
    return citizens;
}

int CitizenManager::getCitizenCount() const
{
    return static_cast<int>(citizens.size());
}

int CitizenManager::getNextId() const
{
    return nextId;
}

void CitizenManager::restoreSaved(std::vector<Citizen> citizens_, int nextId_)
{
    citizens = std::move(citizens_);
    // nextId must stay above every live ID so future citizens never clash.
    // nextId is monotonic across the whole session, so it can legitimately
    // exceed MAX_CITIZENS; only the live count is bounded, not this counter.
    int maxId = 0;
    for (const Citizen& c : citizens)
    {
        if (c.id > maxId)
        {
            maxId = c.id;
        }
    }
    // Guard maxId + 1 against overflow at the top of the int range.
    const int floorId = (maxId < std::numeric_limits<int>::max()) ? (maxId + 1) : maxId;
    nextId = std::max(nextId_, floorId);
}

}  // namespace urbania
