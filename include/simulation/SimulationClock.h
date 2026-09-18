#pragma once

// Simulation clock, fully separate from real-world time and frame rate.
// Real deltaTime flows in through update() and comes out scaled as
// simulation time. All future simulation systems must consume the
// simulation delta from this clock instead of GetFrameTime() directly,
// so the whole city pauses and accelerates consistently.
class SimulationClock {
public:
    // At 1x speed, one real second advances the city by one sim minute.
    static constexpr float SIM_SECONDS_PER_REAL_SECOND = 60.0f;
    static constexpr float NORMAL_SPEED = 1.0f;
    static constexpr float FAST_SPEED = 2.0f;
    static constexpr float VERY_FAST_SPEED = 4.0f;
    static constexpr float EXTREMELY_FAST_SPEED = 8.0f;

    // City starts at Day 1, 08:00.
    static constexpr int START_DAY = 1;
    static constexpr int START_HOUR = 8;
    static constexpr int START_MINUTE = 0;

    SimulationClock();

    void update(float deltaTime);

    void pause();
    void resume();

    void setTimeScale(float scale);
    float getTimeScale() const;

    float getSimulationTime() const;
    float getSimulationDeltaTime() const;
    bool isPaused() const;

    int getDay() const;
    int getHour() const;
    int getMinute() const;

private:
    float simulationTime;
    float timeScale;
    bool paused;
    float lastDeltaTime;
};
