#include "simulation/Employment.h"

#include <map>
#include <utility>

#include "simulation/CitizenManager.h"
#include "world/Tile.h"
#include "world/World.h"

namespace urbania {

void Employment::update(World& world, CitizenManager& citizens, float simulationDeltaTime)
{
    (void)simulationDeltaTime;
    // Matching is driven by world/citizen state, not accumulated time:
    // whenever homes, workplaces, or citizens change, the next update
    // reconciles everything. Time scaling still governs the Population
    // growth that feeds new job seekers in.

    syncJobsWithWorld(world);
    reconcileOccupancy(citizens);
    matchUnemployed(citizens);

    employedCitizens = 0;
    for (const Citizen& citizen : citizens.getCitizens())
    {
        if (citizen.employed)
        {
            ++employedCitizens;
        }
    }
    unemployedCitizens = citizens.getCitizenCount() - employedCitizens;
}

int Employment::getTotalJobs() const
{
    return static_cast<int>(jobs.size());
}

int Employment::getOccupiedJobs() const
{
    int count = 0;
    for (const Job& job : jobs)
    {
        if (job.occupied)
        {
            ++count;
        }
    }
    return count;
}

int Employment::getEmployedCitizens() const
{
    return employedCitizens;
}

int Employment::getUnemployedCitizens() const
{
    return unemployedCitizens;
}

const std::vector<Job>& Employment::getJobs() const
{
    return jobs;
}

int Employment::jobsForTileType(TileType type)
{
    switch (type)
    {
        case TileType::Commercial:
            return JOBS_PER_COMMERCIAL_TILE;
        case TileType::Industrial:
            return JOBS_PER_INDUSTRIAL_TILE;
        default:
            return 0;
    }
}

void Employment::syncJobsWithWorld(const World& world)
{
    // Desired job count per workplace tile, derived from the World.
    std::map<std::pair<int, int>, int> desired;
    for (int y = 0; y < world.getHeight(); ++y)
    {
        for (int x = 0; x < world.getWidth(); ++x)
        {
            const int count = jobsForTileType(world.getTile(x, y).type);
            if (count > 0)
            {
                desired[{ x, y }] = count;
            }
        }
    }

    // Group existing jobs by tile.
    std::map<std::pair<int, int>, int> existing;
    for (const Job& job : jobs)
    {
        existing[{ job.workplace.x, job.workplace.y }] += 1;
    }

    // Drop tiles that no longer provide jobs (demolished or converted).
    // Occupancy is repaired afterwards by reconcileOccupancy().
    std::vector<Job> kept;
    kept.reserve(jobs.size());
    for (const Job& job : jobs)
    {
        const auto it = desired.find({ job.workplace.x, job.workplace.y });
        if (it != desired.end() && existing[{ job.workplace.x, job.workplace.y }] == it->second)
        {
            kept.push_back(job);
        }
    }
    jobs = kept;

    // Recount what survived, then top up tiles that need more jobs.
    existing.clear();
    for (const Job& job : jobs)
    {
        existing[{ job.workplace.x, job.workplace.y }] += 1;
    }
    for (const auto& entry : desired)
    {
        int have = 0;
        const auto it = existing.find(entry.first);
        if (it != existing.end())
        {
            have = it->second;
        }
        for (int i = have; i < entry.second; ++i)
        {
            Job job;
            job.id = nextJobId;
            job.workplace = { entry.first.first, entry.first.second, true };
            ++nextJobId;
            jobs.push_back(job);
        }
    }
}

void Employment::reconcileOccupancy(CitizenManager& citizens)
{
    // Rebuild occupancy from live citizens: jobs held by removed
    // citizens (demolished homes) become vacant, and citizens whose
    // workplace tile lost its jobs become unemployed and rematchable.
    for (Job& job : jobs)
    {
        job.occupied = false;
    }

    for (const Citizen& citizen : citizens.getCitizens())
    {
        if (!citizen.employed || !citizen.workplace.valid)
        {
            continue;
        }

        bool placed = false;
        for (Job& job : jobs)
        {
            if (!job.occupied && job.workplace.x == citizen.workplace.x &&
                job.workplace.y == citizen.workplace.y)
            {
                job.occupied = true;
                placed = true;
                break;
            }
        }

        if (!placed)
        {
            Citizen* mutableCitizen = citizens.getCitizen(citizen.id);
            if (mutableCitizen != nullptr)
            {
                mutableCitizen->employed = false;
                mutableCitizen->workplace = TileCoordinate{};
            }
        }
    }
}

void Employment::matchUnemployed(CitizenManager& citizens)
{
    // First unemployed citizen to first vacant job. Citizen vector order
    // is creation (ID) order and job vector order is creation order, so
    // matching is fully deterministic.
    for (const Citizen& citizen : citizens.getCitizens())
    {
        if (citizen.employed)
        {
            continue;
        }

        for (Job& job : jobs)
        {
            if (!job.occupied)
            {
                Citizen* worker = citizens.getCitizen(citizen.id);
                if (worker != nullptr)
                {
                    worker->employed = true;
                    worker->workplace = job.workplace;
                    job.occupied = true;
                }
                break;
            }
        }
    }
}

}  // namespace urbania
