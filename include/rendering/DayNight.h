#pragma once

namespace urbania {

// Rendering-only day/night atmosphere state, driven by the existing
// SimulationClock hour/minute. Pure math, no raylib dependency, so the
// phase logic stays unit-testable and the overlay stays a thin pass in
// Game::drawDayNight().
//
//   06:00-08:00 -> Dawn (warm tint fading in/out, darkness lifting)
//   08:00-18:00 -> Day (no tint)
//   18:00-20:00 -> Dusk (warm tint, darkness settling)
//   20:00-06:00 -> Night (cool dark tint + lit windows)
enum class DayPhase
{
    Day,
    Dawn,
    Dusk,
    Night
};

struct DayNightState
{
    DayPhase phase = DayPhase::Day;
    float nightFactor = 0.0f;  // 0 in daylight, 1 at full night
    float warmFactor = 0.0f;   // dawn/dusk warmth, peaks at 1
};

inline float dayNightSmooth(float edge0, float edge1, float x)
{
    if (x <= edge0)
    {
        return 0.0f;
    }
    if (x >= edge1)
    {
        return 1.0f;
    }
    const float t = (x - edge0) / (edge1 - edge0);
    return t * t * (3.0f - 2.0f * t);
}

// tHours in [0, 24). All transitions are smooth; exact boundary values
// resolve to the incoming phase (06:00 is Night, 08:00 is Day, ...).
inline DayNightState dayNightAt(float tHours)
{
    DayNightState s;

    if (tHours >= 20.0f || tHours < 6.0f)
    {
        s.nightFactor = 1.0f;
    }
    else if (tHours < 8.0f)
    {
        s.nightFactor = 1.0f - dayNightSmooth(6.0f, 8.0f, tHours);
    }
    else if (tHours < 18.0f)
    {
        s.nightFactor = 0.0f;
    }
    else
    {
        s.nightFactor = dayNightSmooth(18.0f, 20.0f, tHours);
    }

    if (tHours >= 6.0f && tHours < 8.0f)
    {
        s.warmFactor = (tHours <= 7.0f) ? (tHours - 6.0f) : (8.0f - tHours);
        s.phase = DayPhase::Dawn;
    }
    else if (tHours >= 8.0f && tHours < 18.0f)
    {
        s.phase = DayPhase::Day;
    }
    else if (tHours >= 18.0f && tHours < 20.0f)
    {
        s.warmFactor = (tHours <= 19.0f) ? (tHours - 18.0f) : (20.0f - tHours);
        s.phase = DayPhase::Dusk;
    }
    else
    {
        s.phase = DayPhase::Night;
    }

    return s;
}

}  // namespace urbania
