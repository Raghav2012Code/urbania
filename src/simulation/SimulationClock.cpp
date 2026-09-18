#include "simulation/SimulationClock.h"

namespace {

constexpr int SECONDS_PER_MINUTE = 60;
constexpr int SECONDS_PER_HOUR = 3600;
constexpr int SECONDS_PER_DAY = 86400;

bool isSupportedSpeed(float scale)
{
    return scale == SimulationClock::NORMAL_SPEED || scale == SimulationClock::FAST_SPEED ||
           scale == SimulationClock::VERY_FAST_SPEED ||
           scale == SimulationClock::EXTREMELY_FAST_SPEED;
}

}  // namespace

SimulationClock::SimulationClock()
    : simulationTime(static_cast<float>((START_HOUR * SECONDS_PER_HOUR) +
                                       (START_MINUTE * SECONDS_PER_MINUTE))),
      timeScale(NORMAL_SPEED),
      paused(false),
      lastDeltaTime(0.0f)
{
}

void SimulationClock::update(float deltaTime)
{
    lastDeltaTime = deltaTime;

    if (paused)
    {
        return;
    }

    simulationTime += deltaTime * timeScale * SIM_SECONDS_PER_REAL_SECOND;
}

void SimulationClock::pause()
{
    paused = true;
}

void SimulationClock::resume()
{
    paused = false;
}

void SimulationClock::setTimeScale(float scale)
{
    // Only the supported speeds are accepted; anything else is ignored
    // so the clock can never enter an invalid state.
    if (!isSupportedSpeed(scale))
    {
        return;
    }

    timeScale = scale;
    resume();
}

float SimulationClock::getTimeScale() const
{
    return timeScale;
}

float SimulationClock::getSimulationTime() const
{
    return simulationTime;
}

float SimulationClock::getSimulationDeltaTime() const
{
    // Scaled simulation delta for the last frame: real deltaTime x
    // timeScale, expressed in simulation seconds. Zero while paused.
    if (paused)
    {
        return 0.0f;
    }

    return lastDeltaTime * timeScale * SIM_SECONDS_PER_REAL_SECOND;
}

bool SimulationClock::isPaused() const
{
    return paused;
}

int SimulationClock::getDay() const
{
    return START_DAY + static_cast<int>(simulationTime) / SECONDS_PER_DAY;
}

int SimulationClock::getHour() const
{
    return (static_cast<int>(simulationTime) / SECONDS_PER_HOUR) % 24;
}

int SimulationClock::getMinute() const
{
    return (static_cast<int>(simulationTime) / SECONDS_PER_MINUTE) % 60;
}
