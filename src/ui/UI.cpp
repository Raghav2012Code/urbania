#include "ui/UI.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

#include "core/SelfTest.h"
#include "simulation/Economy.h"
#include "simulation/Housing.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationClock.h"
#include "world/Tile.h"
#include "world/World.h"

namespace urbania {

static std::string formatRupees(int amount)
{
    std::string s = std::to_string(std::abs(amount));
    int n = static_cast<int>(s.length()) - 3;
    while (n > 0)
    {
        s.insert(static_cast<size_t>(n), ",");
        n -= 2;
    }
    return (amount < 0 ? "-₹" : "₹") + s;
}

UI::UI()
{
}

bool UI::isMouseOverUI() const
{
    return mouseOverUI;
}

void UI::update(SimulationClock& clock, TileType& selectedBuildType,
                bool& demolishMode, bool& busStopMode, bool& routeMode,
                bool& pollutionOverlay, bool& landValueOverlay, bool& housingOverlay,
                bool& showDashboard, bool& showSelfTestModal)
{
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();
    const Vector2 m = GetMousePosition();

    mouseOverUI = false;

    // 1. Top Ribbon bounds (y: 0 to 48)
    if (m.y >= 0 && m.y <= 48)
    {
        mouseOverUI = true;

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            // Speed Controls
            // Pause (330 to 372)
            if (m.x >= 330 && m.x <= 372 && m.y >= 9 && m.y <= 39)
            {
                if (clock.isPaused()) clock.resume();
                else clock.pause();
            }
            // 1x, 2x, 4x, 8x
            const float speeds[] = { 1.0f, 2.0f, 4.0f, 8.0f };
            for (int i = 0; i < 4; ++i)
            {
                const int bx = 378 + i * 40;
                if (m.x >= bx && m.x <= bx + 36 && m.y >= 9 && m.y <= 39)
                {
                    clock.setTimeScale(speeds[i]);
                    if (clock.isPaused()) clock.resume();
                }
            }

            // Overlay Pills
            if (m.x >= sw - 380 && m.x <= sw - 310 && m.y >= 9 && m.y <= 39) pollutionOverlay = !pollutionOverlay;
            if (m.x >= sw - 302 && m.x <= sw - 232 && m.y >= 9 && m.y <= 39) landValueOverlay = !landValueOverlay;
            if (m.x >= sw - 224 && m.x <= sw - 150 && m.y >= 9 && m.y <= 39) housingOverlay = !housingOverlay;
            if (m.x >= sw - 142 && m.x <= sw - 74 && m.y >= 9 && m.y <= 39) showDashboard = !showDashboard;
            if (m.x >= sw - 66 && m.x <= sw - 12 && m.y >= 9 && m.y <= 39) showSelfTestModal = !showSelfTestModal;
        }
    }

    // 2. Bottom Tool Dock bounds
    const int dockW = 840;
    const int dockH = 74;
    const int dockX = (sw - dockW) / 2;
    const int dockY = sh - 84;

    if (m.x >= dockX && m.x <= dockX + dockW && m.y >= dockY && m.y <= dockY + dockH)
    {
        mouseOverUI = true;

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            const int btnW = 96;
            const int gap = 6;
            for (int i = 0; i < 8; ++i)
            {
                const int bx = dockX + 12 + i * (btnW + gap);
                const int by = dockY + 7;
                if (m.x >= bx && m.x <= bx + btnW && m.y >= by && m.y <= by + 60)
                {
                    switch (i)
                    {
                    case 0: selectedBuildType = TileType::Road; demolishMode = false; busStopMode = false; routeMode = false; break;
                    case 1: selectedBuildType = TileType::Residential; demolishMode = false; busStopMode = false; routeMode = false; break;
                    case 2: selectedBuildType = TileType::Commercial; demolishMode = false; busStopMode = false; routeMode = false; break;
                    case 3: selectedBuildType = TileType::Industrial; demolishMode = false; busStopMode = false; routeMode = false; break;
                    case 4: selectedBuildType = TileType::Park; demolishMode = false; busStopMode = false; routeMode = false; break;
                    case 5: busStopMode = true; demolishMode = false; routeMode = false; break;
                    case 6: routeMode = true; demolishMode = false; busStopMode = false; break;
                    case 7: demolishMode = true; busStopMode = false; routeMode = false; break;
                    }
                }
            }
        }
    }

    // 3. Right-Side Dashboard bounds
    if (showDashboard)
    {
        const int dw = 350;
        const int dh = 480;
        const int dx = sw - dw - 16;
        const int dy = 58;

        if (m.x >= dx && m.x <= dx + dw && m.y >= dy && m.y <= dy + dh)
        {
            mouseOverUI = true;

            // Tab bar clicks
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && m.y >= dy + 32 && m.y <= dy + 58)
            {
                const int tabW = (dw - 24) / 5;
                for (int t = 0; t < 5; ++t)
                {
                    const int tx = dx + 12 + t * tabW;
                    if (m.x >= tx && m.x <= tx + tabW)
                    {
                        currentTab = static_cast<DashboardTab>(t);
                    }
                }
            }
        }
    }

    // 4. SelfTest Modal bounds
    if (showSelfTestModal)
    {
        mouseOverUI = true;
    }
}

void UI::draw(const World& world, const Simulation& sim,
              const SimulationClock& clock, TileType selectedBuildType,
              bool demolishMode, bool busStopMode, bool routeMode,
              bool pollutionOverlay, bool landValueOverlay, bool housingOverlay,
              bool showDashboard, bool showSelfTestModal,
              const TileCoordinate& hoveredTile,
              const std::vector<int>& currentRouteStops,
              const std::string& toastMessage, float toastTimer,
              const SelfTest& selfTest)
{
    drawTopRibbon(sim, clock, pollutionOverlay, landValueOverlay, housingOverlay,
                  showDashboard, showSelfTestModal);
    drawDemandMeters(sim);
    drawBottomDock(sim, selectedBuildType, demolishMode, busStopMode, routeMode);

    if (showDashboard)
    {
        drawDashboard(world, sim);
    }

    if (hoveredTile.valid && !mouseOverUI)
    {
        drawTileInspector(world, sim, hoveredTile);
    }

    drawOverlayLegends(pollutionOverlay, landValueOverlay, housingOverlay);
    const bool hasBanner = demolishMode || busStopMode || routeMode;
    drawModeBanners(demolishMode, busStopMode, routeMode, currentRouteStops);
    drawToast(toastMessage, toastTimer, hasBanner);

    if (showSelfTestModal)
    {
        bool showModal = true;
        drawSelfTestModal(selfTest, showModal);
    }
}

void UI::drawTopRibbon(const Simulation& sim, const SimulationClock& clock,
                       bool pollutionOverlay, bool landValueOverlay, bool housingOverlay,
                       bool showDashboard, bool showSelfTestModal)
{
    const int sw = GetScreenWidth();
    const int barH = 46;

    // Dark glass ribbon
    DrawRectangle(0, 0, sw, barH, Color{ 14, 18, 28, 248 });
    DrawRectangle(0, barH - 2, sw, 2, Color{ 38, 48, 70, 255 });

    // Logo & Brand
    DrawText("URBANIA", 18, 12, 22, Color{ 245, 250, 255, 255 });
    DrawRectangleRounded(Rectangle{ 124, 15, 46, 18 }, 0.4f, 4, Color{ 35, 65, 115, 255 });
    DrawText("CITY", 133, 18, 11, Color{ 120, 200, 255, 255 });

    // Calendar & Clock Card
    DrawRectangleRounded(Rectangle{ 180, 8, 140, 32 }, 0.25f, 4, Color{ 22, 28, 44, 255 });
    DrawRectangleRoundedLines(Rectangle{ 180, 8, 140, 32 }, 0.25f, 4, Color{ 45, 58, 85, 255 });
    DrawText(TextFormat("Day %d • %02d:%02d", clock.getDay(), clock.getHour(), clock.getMinute()),
             192, 16, 15, Color{ 210, 225, 245, 255 });

    // Time & Speed Controls
    const bool isPaused = clock.isPaused();
    const float speed = clock.getTimeScale();

    // Pause pill
    const Color pauseBg = isPaused ? Color{ 220, 50, 50, 255 } : Color{ 26, 33, 50, 255 };
    const Color pauseText = isPaused ? WHITE : Color{ 150, 165, 190, 255 };
    DrawRectangleRounded(Rectangle{ 330, 9, 42, 30 }, 0.3f, 4, pauseBg);
    DrawRectangleRoundedLines(Rectangle{ 330, 9, 42, 30 }, 0.3f, 4, Color{ 45, 58, 85, 255 });
    DrawText("||", 346, 16, 15, pauseText);

    // Speed 1x, 2x, 4x, 8x
    const float speeds[] = { 1.0f, 2.0f, 4.0f, 8.0f };
    const char* speedLabels[] = { "1×", "2×", "4×", "8×" };
    for (int i = 0; i < 4; ++i)
    {
        const int bx = 378 + i * 40;
        const bool active = (!isPaused && speed == speeds[i]);
        const Color btnBg = active ? Color{ 0, 185, 245, 255 } : Color{ 26, 33, 50, 255 };
        const Color btnText = active ? Color{ 10, 20, 30, 255 } : Color{ 150, 165, 190, 255 };
        DrawRectangleRounded(Rectangle{ static_cast<float>(bx), 9, 36, 30 }, 0.3f, 4, btnBg);
        DrawRectangleRoundedLines(Rectangle{ static_cast<float>(bx), 9, 36, 30 }, 0.3f, 4, Color{ 45, 58, 85, 255 });
        DrawText(speedLabels[i], bx + 8, 16, 14, btnText);
    }

    // Money & Daily Cash Flow (Center)
    const int moneyX = 555;
    DrawRectangleRounded(Rectangle{ static_cast<float>(moneyX), 8, 185, 32 }, 0.25f, 4, Color{ 22, 28, 44, 255 });
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(moneyX), 8, 185, 32 }, 0.25f, 4, Color{ 45, 58, 85, 255 });
    DrawText(formatRupees(sim.getEconomy().getMoney()).c_str(), moneyX + 10, 15, 16, Color{ 46, 204, 113, 255 });

    const int netInc = static_cast<int>(sim.getEconomy().getNetIncome());
    if (netInc >= 0)
    {
        DrawText(TextFormat("+%s/d", formatRupees(netInc).c_str()), moneyX + 112, 17, 12, Color{ 46, 204, 113, 220 });
    }
    else
    {
        DrawText(TextFormat("-%s/d", formatRupees(-netInc).c_str()), moneyX + 112, 17, 12, Color{ 231, 76, 60, 220 });
    }

    // Population & Happiness
    const int popX = 750;
    DrawRectangleRounded(Rectangle{ static_cast<float>(popX), 8, 225, 32 }, 0.25f, 4, Color{ 22, 28, 44, 255 });
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(popX), 8, 225, 32 }, 0.25f, 4, Color{ 45, 58, 85, 255 });
    DrawText(TextFormat("Pop: %d", sim.getPopulation().getTotalPopulation()), popX + 12, 15, 15, WHITE);

    const float happy = sim.getHappiness().getAverageHappiness();
    Color happyCol = Color{ 46, 204, 113, 255 }; // Green
    if (happy < 40.0f) happyCol = Color{ 231, 76, 60, 255 }; // Red
    else if (happy < 65.0f) happyCol = Color{ 241, 196, 15, 255 }; // Yellow

    DrawRectangleRounded(Rectangle{ static_cast<float>(popX + 108), 11, 108, 26 }, 0.3f, 4,
                         Color{ happyCol.r, happyCol.g, happyCol.b, 40 });
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(popX + 108), 11, 108, 26 }, 0.3f, 4, happyCol);
    DrawText(TextFormat("%.1f%% Happy", happy), popX + 116, 16, 13, happyCol);

    // Right-side Overlay & Feature Pills
    const int pillY = 9;
    const int pillH = 30;

    // F5 Smog
    const Color smogBg = pollutionOverlay ? Color{ 210, 105, 30, 255 } : Color{ 26, 33, 50, 255 };
    const Color smogText = pollutionOverlay ? WHITE : Color{ 150, 165, 190, 255 };
    DrawRectangleRounded(Rectangle{ static_cast<float>(sw - 380), static_cast<float>(pillY), 70, static_cast<float>(pillH) }, 0.3f, 4, smogBg);
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(sw - 380), static_cast<float>(pillY), 70, static_cast<float>(pillH) }, 0.3f, 4, Color{ 45, 58, 85, 255 });
    DrawText("F5 Smog", sw - 373, pillY + 7, 13, smogText);

    // F6 Land Value
    const Color landBg = landValueOverlay ? Color{ 46, 204, 113, 255 } : Color{ 26, 33, 50, 255 };
    const Color landText = landValueOverlay ? Color{ 10, 25, 15, 255 } : Color{ 150, 165, 190, 255 };
    DrawRectangleRounded(Rectangle{ static_cast<float>(sw - 302), static_cast<float>(pillY), 70, static_cast<float>(pillH) }, 0.3f, 4, landBg);
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(sw - 302), static_cast<float>(pillY), 70, static_cast<float>(pillH) }, 0.3f, 4, Color{ 45, 58, 85, 255 });
    DrawText("F6 Land", sw - 295, pillY + 7, 13, landText);

    // F7 Housing
    const Color houseBg = housingOverlay ? Color{ 52, 152, 219, 255 } : Color{ 26, 33, 50, 255 };
    const Color houseText = housingOverlay ? WHITE : Color{ 150, 165, 190, 255 };
    DrawRectangleRounded(Rectangle{ static_cast<float>(sw - 224), static_cast<float>(pillY), 74, static_cast<float>(pillH) }, 0.3f, 4, houseBg);
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(sw - 224), static_cast<float>(pillY), 74, static_cast<float>(pillH) }, 0.3f, 4, Color{ 45, 58, 85, 255 });
    DrawText("F7 House", sw - 217, pillY + 7, 13, houseText);

    // TAB Dashboard
    const Color dashBg = showDashboard ? Color{ 142, 68, 173, 255 } : Color{ 26, 33, 50, 255 };
    const Color dashText = showDashboard ? WHITE : Color{ 150, 165, 190, 255 };
    DrawRectangleRounded(Rectangle{ static_cast<float>(sw - 142), static_cast<float>(pillY), 68, static_cast<float>(pillH) }, 0.3f, 4, dashBg);
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(sw - 142), static_cast<float>(pillY), 68, static_cast<float>(pillH) }, 0.3f, 4, Color{ 45, 58, 85, 255 });
    DrawText("TAB Info", sw - 135, pillY + 7, 13, dashText);

    // F9 SelfTest
    const Color testBg = showSelfTestModal ? Color{ 230, 126, 34, 255 } : Color{ 26, 33, 50, 255 };
    const Color testText = showSelfTestModal ? WHITE : Color{ 150, 165, 190, 255 };
    DrawRectangleRounded(Rectangle{ static_cast<float>(sw - 66), static_cast<float>(pillY), 54, static_cast<float>(pillH) }, 0.3f, 4, testBg);
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(sw - 66), static_cast<float>(pillY), 54, static_cast<float>(pillH) }, 0.3f, 4, Color{ 45, 58, 85, 255 });
    DrawText("F9 Test", sw - 60, pillY + 7, 13, testText);
}

void UI::drawDemandMeters(const Simulation& sim)
{
    const int x = 16;
    const int y = 58;
    const int w = 150;
    const int h = 164;

    DrawRectangleRounded(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                         0.1f, 4, Color{ 14, 18, 28, 235 });
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                              0.1f, 4, Color{ 40, 52, 75, 255 });

    DrawText("RCI DEMAND", x + 30, y + 9, 13, Color{ 200, 215, 235, 255 });
    DrawLine(x + 12, y + 26, x + w - 12, y + 26, Color{ 40, 52, 75, 255 });

    const int baseY = y + 90;
    const int colW = 28;
    const int maxBarH = 45;

    DrawLine(x + 16, baseY, x + w - 16, baseY, Color{ 70, 85, 115, 255 });

    // Residential (R)
    const int rDem = sim.getDemand().getResidentialDemand();
    const int rX = x + 22;
    const float rFrac = std::clamp(static_cast<float>(rDem) / 100.0f, -1.0f, 1.0f);
    const int rH = static_cast<int>(std::abs(rFrac) * maxBarH);
    if (rFrac >= 0) DrawRectangle(rX, baseY - rH, colW, rH, Color{ 46, 204, 113, 230 });
    else DrawRectangle(rX, baseY, colW, rH, Color{ 39, 174, 96, 120 });
    DrawRectangleLines(rX, baseY - maxBarH, colW, maxBarH * 2, Color{ 46, 204, 113, 80 });
    DrawText("R", rX + 9, baseY + maxBarH + 5, 13, Color{ 46, 204, 113, 255 });
    DrawText(TextFormat("%+d", rDem), rX + (rDem >= 0 ? 3 : 1), (rFrac >= 0 ? baseY - rH - 13 : baseY + rH + 2), 10, Color{ 210, 235, 220, 255 });

    // Commercial (C)
    const int cDem = sim.getDemand().getCommercialDemand();
    const int cX = x + 62;
    const float cFrac = std::clamp(static_cast<float>(cDem) / 100.0f, -1.0f, 1.0f);
    const int cH = static_cast<int>(std::abs(cFrac) * maxBarH);
    if (cFrac >= 0) DrawRectangle(cX, baseY - cH, colW, cH, Color{ 52, 152, 219, 230 });
    else DrawRectangle(cX, baseY, colW, cH, Color{ 41, 128, 185, 120 });
    DrawRectangleLines(cX, baseY - maxBarH, colW, maxBarH * 2, Color{ 52, 152, 219, 80 });
    DrawText("C", cX + 8, baseY + maxBarH + 5, 13, Color{ 52, 152, 219, 255 });
    DrawText(TextFormat("%+d", cDem), cX + (cDem >= 0 ? 3 : 1), (cFrac >= 0 ? baseY - cH - 13 : baseY + cH + 2), 10, Color{ 210, 230, 255, 255 });

    // Industrial (I)
    const int iDem = sim.getDemand().getIndustrialDemand();
    const int iX = x + 102;
    const float iFrac = std::clamp(static_cast<float>(iDem) / 100.0f, -1.0f, 1.0f);
    const int iH = static_cast<int>(std::abs(iFrac) * maxBarH);
    if (iFrac >= 0) DrawRectangle(iX, baseY - iH, colW, iH, Color{ 230, 126, 34, 230 });
    else DrawRectangle(iX, baseY, colW, iH, Color{ 211, 84, 0, 120 });
    DrawRectangleLines(iX, baseY - maxBarH, colW, maxBarH * 2, Color{ 230, 126, 34, 80 });
    DrawText("I", iX + 10, baseY + maxBarH + 5, 13, Color{ 230, 126, 34, 255 });
    DrawText(TextFormat("%+d", iDem), iX + (iDem >= 0 ? 3 : 1), (iFrac >= 0 ? baseY - iH - 13 : baseY + iH + 2), 10, Color{ 255, 230, 210, 255 });
}

void UI::drawBottomDock(const Simulation& sim, TileType selectedBuildType,
                        bool demolishMode, bool busStopMode, bool routeMode)
{
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();
    const int dockW = 840;
    const int dockH = 74;
    const int dockX = (sw - dockW) / 2;
    const int dockY = sh - 84;

    DrawRectangleRounded(Rectangle{ static_cast<float>(dockX), static_cast<float>(dockY),
                                   static_cast<float>(dockW), static_cast<float>(dockH) },
                         0.2f, 4, Color{ 14, 18, 28, 245 });
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(dockX), static_cast<float>(dockY),
                                        static_cast<float>(dockW), static_cast<float>(dockH) },
                              0.2f, 4, Color{ 45, 60, 90, 255 });

    struct ToolItem {
        const char* key;
        const char* name;
        const char* cost;
        const char* desc;
        Color swatch;
        bool active;
        bool affordable;
    };

    const ToolItem tools[8] = {
        { "[1]", "Road", "₹100", "Lays road pavement for citizen commutes & bus lines", Color{ 130, 135, 145, 255 },
          !demolishMode && !busStopMode && !routeMode && selectedBuildType == TileType::Road,
          sim.getEconomy().canAfford(TileType::Road) },
        { "[2]", "Resi", "₹2,000", "Zones residential plots for citizens to build homes", Color{ 70, 130, 220, 255 },
          !demolishMode && !busStopMode && !routeMode && selectedBuildType == TileType::Residential,
          sim.getEconomy().canAfford(TileType::Residential) },
        { "[3]", "Comm", "₹5,000", "Zones commercial services and shops for city revenue", Color{ 240, 160, 40, 255 },
          !demolishMode && !busStopMode && !routeMode && selectedBuildType == TileType::Commercial,
          sim.getEconomy().canAfford(TileType::Commercial) },
        { "[4]", "Ind", "₹10,000", "Zones factories creating jobs (generates heavy smog)", Color{ 175, 75, 75, 255 },
          !demolishMode && !busStopMode && !routeMode && selectedBuildType == TileType::Industrial,
          sim.getEconomy().canAfford(TileType::Industrial) },
        { "[5]", "Park", "₹1,000", "Plants city parks (+15 land value boost, filters smog)", Color{ 35, 140, 60, 255 },
          !demolishMode && !busStopMode && !routeMode && selectedBuildType == TileType::Park,
          sim.getEconomy().canAfford(TileType::Park) },
        { "[B]", "Bus Stop", "₹500", "Installs transit stops on roads for bus routes", Color{ 255, 215, 0, 255 },
          busStopMode,
          sim.getEconomy().canAfford(urbania::Transit::BUS_STOP_COST) },
        { "[R]", "Route", "Transit", "Connects placed bus stops into active commuter routes", Color{ 180, 50, 220, 255 },
          routeMode,
          true },
        { "[D]", "Demolish", "Free", "Clears structures and roads back to open grass", Color{ 220, 50, 50, 255 },
          demolishMode,
          true }
    };

    const int btnW = 96;
    const int btnH = 60;
    const int gap = 6;
    const Vector2 mouse = GetMousePosition();
    int hoveredIdx = -1;

    for (int i = 0; i < 8; ++i)
    {
        const int bx = dockX + 12 + i * (btnW + gap);
        const int by = dockY + 7;
        const bool hovered = (mouse.x >= bx && mouse.x <= bx + btnW && mouse.y >= by && mouse.y <= by + btnH);
        const bool selected = tools[i].active;
        if (hovered) hoveredIdx = i;

        Color btnBg = selected ? Color{ 35, 48, 75, 255 } : (hovered ? Color{ 25, 34, 52, 255 } : Color{ 18, 24, 38, 255 });
        Color borderColor = selected ? Color{ 255, 215, 0, 255 } : (hovered ? Color{ 100, 130, 180, 255 } : Color{ 35, 48, 70, 255 });

        DrawRectangleRounded(Rectangle{ static_cast<float>(bx), static_cast<float>(by),
                                       static_cast<float>(btnW), static_cast<float>(btnH) },
                             0.2f, 4, btnBg);
        DrawRectangleRoundedLines(Rectangle{ static_cast<float>(bx), static_cast<float>(by),
                                            static_cast<float>(btnW), static_cast<float>(btnH) },
                                  0.2f, 4, borderColor);

        // Color Swatch Badge
        DrawRectangleRounded(Rectangle{ static_cast<float>(bx + 8), static_cast<float>(by + 8), 16, 16 }, 0.3f, 2, tools[i].swatch);

        // Hotkey tag
        DrawText(tools[i].key, bx + 30, by + 8, 12, Color{ 130, 155, 195, 255 });

        // Name
        Color nameCol = selected ? WHITE : (tools[i].affordable ? Color{ 210, 220, 235, 255 } : Color{ 130, 140, 155, 255 });
        DrawText(tools[i].name, bx + 8, by + 28, 13, nameCol);

        // Price
        Color costCol = tools[i].affordable ? Color{ 46, 204, 113, 220 } : Color{ 231, 76, 60, 240 };
        DrawText(tools[i].cost, bx + 8, by + 43, 11, costCol);
    }

    // Floating Tooltip above dock when hovered
    if (hoveredIdx >= 0)
    {
        const auto& t = tools[hoveredIdx];
        std::string tipText = std::string(t.name) + " " + t.key + " • " + t.cost;
        if (!t.affordable) tipText += " (Not Enough Funds)";
        tipText += " — " + std::string(t.desc);

        const int tipW = MeasureText(tipText.c_str(), 12) + 24;
        const int tipH = 26;
        const int tipX = std::clamp(static_cast<int>(mouse.x) - tipW / 2, 16, sw - tipW - 16);
        const int tipY = dockY - tipH - 6;

        DrawRectangleRounded(Rectangle{ static_cast<float>(tipX), static_cast<float>(tipY),
                                       static_cast<float>(tipW), static_cast<float>(tipH) },
                             0.3f, 4, Color{ 10, 14, 22, 240 });
        DrawRectangleRoundedLines(Rectangle{ static_cast<float>(tipX), static_cast<float>(tipY),
                                            static_cast<float>(tipW), static_cast<float>(tipH) },
                                  0.3f, 4, Color{ 60, 80, 115, 255 });
        DrawText(tipText.c_str(), tipX + 12, tipY + 7, 12, t.affordable ? Color{ 230, 240, 255, 255 } : Color{ 255, 170, 170, 255 });
    }
}

void UI::drawDashboard(const World& world, const Simulation& sim)
{
    (void)world;
    const int sw = GetScreenWidth();
    const int w = 350;
    const int h = 480;
    const int x = sw - w - 16;
    const int y = 58;

    DrawRectangleRounded(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                         0.05f, 4, Color{ 14, 18, 28, 245 });
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                              0.05f, 4, Color{ 40, 52, 75, 255 });

    // Header
    DrawText("CITY DASHBOARD", x + 16, y + 12, 14, Color{ 200, 215, 240, 255 });
    DrawText("[TAB to Hide]", x + w - 95, y + 14, 11, Color{ 120, 140, 175, 255 });

    // Tab switcher
    const char* tabNames[5] = { "Overview", "Economy", "People", "Transit", "Eco" };
    const int tabW = (w - 24) / 5;
    for (int t = 0; t < 5; ++t)
    {
        const int tx = x + 12 + t * tabW;
        const int ty = y + 34;
        const bool active = (static_cast<int>(currentTab) == t);
        const Color tabBg = active ? Color{ 35, 65, 115, 255 } : Color{ 22, 28, 42, 255 };
        const Color tabTxt = active ? WHITE : Color{ 140, 155, 180, 255 };

        DrawRectangleRounded(Rectangle{ static_cast<float>(tx), static_cast<float>(ty), static_cast<float>(tabW - 2), 24 }, 0.2f, 2, tabBg);
        DrawText(tabNames[t], tx + 4, ty + 6, 11, tabTxt);
    }

    DrawLine(x + 12, y + 64, x + w - 12, y + 64, Color{ 35, 48, 70, 255 });

    int cy = y + 74;

    switch (currentTab)
    {
    case DashboardTab::Overview:
    {
        DrawText("CITY OVERVIEW", x + 16, cy, 12, Color{ 0, 180, 240, 255 });
        cy += 20;
        DrawText(TextFormat("Total Population: %d citizens", sim.getPopulation().getTotalPopulation()), x + 16, cy, 13, WHITE);
        cy += 20;
        DrawText(TextFormat("Housing Capacity: %d / %d (%.0f%% full)", sim.getHousing().getTotalResidents(), sim.getHousing().getTotalCapacity(), sim.getHousing().getOccupancyRatio() * 100.0f), x + 16, cy, 13, Color{ 190, 205, 225, 255 });
        cy += 20;
        DrawText(TextFormat("Average Happiness: %.1f%%", sim.getHappiness().getAverageHappiness()), x + 16, cy, 13, Color{ 46, 204, 113, 255 });
        cy += 20;
        DrawText(TextFormat("Average Land Value: %.1f / 100", sim.getLandValue().getAverageLandValue()), x + 16, cy, 13, Color{ 190, 205, 225, 255 });
        cy += 20;
        DrawText(TextFormat("Average Pollution: %.1f ppm", sim.getPollution().getAveragePollution()), x + 16, cy, 13, Color{ 230, 126, 34, 255 });
        cy += 26;

        const int net = static_cast<int>(sim.getEconomy().getNetIncome());
        DrawText("DAILY CASH FLOW", x + 16, cy, 12, Color{ 0, 180, 240, 255 });
        cy += 20;
        DrawText(TextFormat("Net Flow: %s%s / day", (net >= 0 ? "+" : "-"), formatRupees(std::abs(net)).c_str()),
                 x + 16, cy, 14, (net >= 0 ? Color{ 46, 204, 113, 255 } : Color{ 231, 76, 60, 255 }));
        break;
    }

    case DashboardTab::Economy:
    {
        DrawText("FINANCES & LEDGER", x + 16, cy, 12, Color{ 46, 204, 113, 255 });
        cy += 20;
        DrawText(TextFormat("Treasury Balance: %s", formatRupees(sim.getEconomy().getMoney()).c_str()), x + 16, cy, 13, WHITE);
        cy += 20;
        DrawText(TextFormat("Daily Tax Revenue: +%s", formatRupees(static_cast<int>(sim.getEconomy().getTaxIncome())).c_str()), x + 16, cy, 13, Color{ 46, 204, 113, 255 });
        cy += 20;
        DrawText(TextFormat("Municipal Upkeep: -%s", formatRupees(static_cast<int>(sim.getEconomy().getMaintenanceCost())).c_str()), x + 16, cy, 13, Color{ 231, 76, 60, 255 });
        cy += 24;

        const int net = static_cast<int>(sim.getEconomy().getNetIncome());
        DrawText(TextFormat("Net Daily Margin: %s%s / day", (net >= 0 ? "+" : "-"), formatRupees(std::abs(net)).c_str()),
                 x + 16, cy, 14, (net >= 0 ? Color{ 46, 204, 113, 255 } : Color{ 231, 76, 60, 255 }));
        cy += 28;

        DrawText("ZONE DEMAND INDICES", x + 16, cy, 12, Color{ 0, 180, 240, 255 });
        cy += 20;
        DrawText(TextFormat("Residential Demand: %+d", sim.getDemand().getResidentialDemand()), x + 16, cy, 13, Color{ 46, 204, 113, 255 });
        cy += 18;
        DrawText(TextFormat("Commercial Demand: %+d", sim.getDemand().getCommercialDemand()), x + 16, cy, 13, Color{ 52, 152, 219, 255 });
        cy += 18;
        DrawText(TextFormat("Industrial Demand: %+d", sim.getDemand().getIndustrialDemand()), x + 16, cy, 13, Color{ 230, 126, 34, 255 });
        break;
    }

    case DashboardTab::Population:
    {
        DrawText("CITIZENS & WORKFORCE", x + 16, cy, 12, Color{ 52, 152, 219, 255 });
        cy += 20;
        DrawText(TextFormat("Total Citizens: %d", sim.getPopulation().getTotalPopulation()), x + 16, cy, 13, WHITE);
        cy += 20;
        DrawText(TextFormat("Employed: %d • Unemployed: %d", sim.getEmployment().getEmployedCitizens(), sim.getEmployment().getUnemployedCitizens()), x + 16, cy, 13, Color{ 190, 205, 225, 255 });
        cy += 20;
        DrawText(TextFormat("Workplace Jobs: %d / %d filled", sim.getEmployment().getOccupiedJobs(), sim.getEmployment().getTotalJobs()), x + 16, cy, 13, Color{ 190, 205, 225, 255 });
        cy += 26;

        DrawText("RESIDENTIAL CAPACITY", x + 16, cy, 12, Color{ 0, 180, 240, 255 });
        cy += 20;
        DrawText(TextFormat("Capacity: %d beds • %d residents", sim.getHousing().getTotalCapacity(), sim.getHousing().getTotalResidents()), x + 16, cy, 13, Color{ 190, 205, 225, 255 });
        cy += 20;
        DrawText(TextFormat("Housing Pressure: %+d", sim.getHousing().getHousingPressure()), x + 16, cy, 13, (sim.getHousing().getHousingPressure() > 0 ? Color{ 241, 196, 15, 255 } : Color{ 140, 160, 190, 255 }));
        break;
    }

    case DashboardTab::Transit:
    {
        DrawText("MOBILITY & TRANSIT", x + 16, cy, 12, Color{ 230, 126, 34, 255 });
        cy += 20;
        DrawText(TextFormat("Road Network: %d road nodes", sim.getRoadNetwork().getNodeCount()), x + 16, cy, 13, WHITE);
        cy += 20;
        DrawText(TextFormat("Private Vehicles: %d active cars", sim.getTraffic().getActiveVehicleCount()), x + 16, cy, 13, Color{ 190, 205, 225, 255 });
        cy += 20;
        DrawText(TextFormat("Congested Roads: %d (Max: %.1fx)", sim.getCongestion().getCongestedRoadCount(), sim.getCongestion().getMaxCongestion()), x + 16, cy, 13, Color{ 190, 205, 225, 255 });
        cy += 26;

        DrawText("PUBLIC BUS TRANSIT", x + 16, cy, 12, Color{ 255, 215, 0, 255 });
        cy += 20;
        DrawText(TextFormat("Bus Stops: %d placed", sim.getTransit().getBusStopCount()), x + 16, cy, 13, Color{ 190, 205, 225, 255 });
        cy += 18;
        DrawText(TextFormat("Bus Routes: %d configured", sim.getTransit().getRouteCount()), x + 16, cy, 13, Color{ 190, 205, 225, 255 });
        cy += 18;
        DrawText(TextFormat("Buses: %d active / %d total", sim.getTransit().getActiveBusCount(), sim.getTransit().getBusCount()), x + 16, cy, 13, Color{ 255, 215, 0, 255 });
        cy += 18;
        DrawText(TextFormat("Commuters: %d routed • %d unrouted", sim.getCommuteSystem().getRoutedCitizens(), sim.getCommuteSystem().getUnroutedCitizens()), x + 16, cy, 13, Color{ 190, 205, 225, 255 });
        break;
    }

    case DashboardTab::Environment:
    {
        DrawText("ENVIRONMENT & LAND VALUE", x + 16, cy, 12, Color{ 46, 204, 113, 255 });
        cy += 20;
        DrawText(TextFormat("Average Pollution: %.1f ppm", sim.getPollution().getAveragePollution()), x + 16, cy, 13, Color{ 230, 126, 34, 255 });
        cy += 20;
        DrawText(TextFormat("Max Pollution Hotspot: %.1f ppm", sim.getPollution().getMaxPollution()), x + 16, cy, 13, Color{ 231, 76, 60, 255 });
        cy += 20;
        DrawText(TextFormat("Average Land Desirability: %.1f / 100", sim.getLandValue().getAverageLandValue()), x + 16, cy, 13, Color{ 46, 204, 113, 255 });
        cy += 26;

        DrawText("HAPPINESS IMPACT FACTORS", x + 16, cy, 12, Color{ 0, 180, 240, 255 });
        cy += 20;
        DrawText("• Industrial zones create smog plumes", x + 16, cy, 12, Color{ 190, 205, 225, 255 });
        cy += 18;
        DrawText("• Parks clean smog & boost nearby Land Value", x + 16, cy, 12, Color{ 190, 205, 225, 255 });
        cy += 18;
        DrawText("• Heavy traffic congestion slows commutes", x + 16, cy, 12, Color{ 190, 205, 225, 255 });
        break;
    }
    }
}

void UI::drawTileInspector(const World& world, const Simulation& sim,
                           const TileCoordinate& hovered)
{
    const int sw = GetScreenWidth();
    const int w = 260;
    const int h = 175;
    const int x = sw - w - 16;
    const int y = GetScreenHeight() - h - 94;

    DrawRectangleRounded(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                         0.1f, 4, Color{ 14, 18, 28, 240 });
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                              0.1f, 4, Color{ 40, 52, 75, 255 });

    const Tile& t = world.getTile(hovered.x, hovered.y);
    int cy = y + 10;

    // Header with Zone Badge
    DrawText(TextFormat("TILE (%d, %d)", hovered.x, hovered.y), x + 12, cy, 13, WHITE);

    Color badgeCol = Color{ 48, 140, 68, 255 };
    const char* typeName = "Grass";
    switch (t.type)
    {
    case TileType::Road: badgeCol = Color{ 120, 125, 135, 255 }; typeName = "Road"; break;
    case TileType::Residential: badgeCol = Color{ 70, 130, 220, 255 }; typeName = "Residential"; break;
    case TileType::Commercial: badgeCol = Color{ 240, 160, 40, 255 }; typeName = "Commercial"; break;
    case TileType::Industrial: badgeCol = Color{ 175, 75, 75, 255 }; typeName = "Industrial"; break;
    case TileType::Park: badgeCol = Color{ 35, 140, 60, 255 }; typeName = "Park"; break;
    default: break;
    }

    DrawRectangleRounded(Rectangle{ static_cast<float>(x + 140), static_cast<float>(cy - 2), 108, 18 }, 0.3f, 4, badgeCol);
    DrawText(typeName, x + 148, cy + 1, 11, Color{ 10, 20, 30, 255 });
    cy += 22;

    DrawLine(x + 10, cy, x + w - 10, cy, Color{ 35, 48, 70, 255 });
    cy += 10;

    // Land Value
    const float lv = sim.getLandValue().getLandValue(hovered.x, hovered.y);
    DrawText(TextFormat("Land Value: %.1f / 100", lv), x + 12, cy, 12, Color{ 190, 205, 225, 255 });
    cy += 18;

    // Specific tile metrics
    if (t.type == TileType::Residential)
    {
        const int res = sim.getPopulation().getResidentsAt(hovered.x, hovered.y);
        DrawText(TextFormat("Residents: %d / %d", res, Housing::CAPACITY_PER_TILE), x + 12, cy, 12, Color{ 52, 152, 219, 255 });
        cy += 18;
    }
    else if (t.type == TileType::Commercial)
    {
        DrawText("Commercial Shop: Active", x + 12, cy, 12, Color{ 240, 160, 40, 255 });
        cy += 18;
    }
    else if (t.type == TileType::Industrial)
    {
        const float p = sim.getPollution().getPollution(hovered.x, hovered.y);
        DrawText(TextFormat("Smog Output: %.1f ppm", p), x + 12, cy, 12, Color{ 230, 126, 34, 255 });
        cy += 18;
    }
    else if (t.type == TileType::Road)
    {
        const int vCount = sim.getCongestion().getVehicleCount(hovered.x, hovered.y);
        const float cong = sim.getCongestion().getCongestion(hovered.x, hovered.y);
        DrawText(TextFormat("Traffic: %d / 5 (%.1fx slow)", vCount, cong), x + 12, cy, 12, (cong > 1.0f ? Color{ 231, 76, 60, 255 } : Color{ 190, 205, 225, 255 }));
        cy += 18;
    }
    else if (t.type == TileType::Park)
    {
        DrawText("Park: +15 Land Value Bonus", x + 12, cy, 12, Color{ 46, 204, 113, 255 });
        cy += 18;
    }

    if (sim.getTransit().hasBusStop(hovered))
    {
        const auto* stop = sim.getTransit().getBusStop(hovered);
        if (stop != nullptr)
        {
            DrawText(TextFormat("Transit: Bus Stop #%d", stop->id), x + 12, cy, 12, Color{ 255, 215, 0, 255 });
            cy += 18;
        }
    }

    for (const auto& bus : sim.getTransit().getBuses())
    {
        if (bus.active && !bus.path.empty() && bus.pathIndex >= 0 &&
            bus.pathIndex < static_cast<int>(bus.path.size()))
        {
            const auto& bTile = bus.path[bus.pathIndex];
            if (bTile.x == hovered.x && bTile.y == hovered.y)
            {
                const auto* r = sim.getTransit().getRoute(bus.routeId);
                const int total = (r != nullptr) ? static_cast<int>(r->stopIds.size()) : 0;
                DrawText(TextFormat("Bus #%d • Route #%d (Stop %d/%d)", bus.id, bus.routeId,
                                    bus.currentStopIndex + 1, total),
                         x + 12, cy, 11, Color{ 255, 215, 0, 255 });
                cy += 18;
                break;
            }
        }
    }

    const float p = sim.getPollution().getPollution(hovered.x, hovered.y);
    if (p > 0.01f)
    {
        DrawText(TextFormat("Air Pollution: %.1f ppm", p), x + 12, cy, 12, Color{ 230, 126, 34, 255 });
    }
}

void UI::drawOverlayLegends(bool pollutionOverlay, bool landValueOverlay, bool housingOverlay)
{
    if (!pollutionOverlay && !landValueOverlay && !housingOverlay)
    {
        return;
    }

    const int x = 16;
    const int y = 230;
    const int w = 150;
    const int h = 76;

    DrawRectangleRounded(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                         0.1f, 4, Color{ 14, 18, 28, 235 });
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                              0.1f, 4, Color{ 40, 52, 75, 255 });

    if (pollutionOverlay)
    {
        DrawText("POLLUTION SMOG", x + 12, y + 8, 11, Color{ 230, 126, 34, 255 });
        DrawRectangleGradientH(x + 12, y + 26, w - 24, 12, Color{ 60, 140, 80, 200 }, Color{ 180, 40, 40, 255 });
        DrawText("0 Clean", x + 12, y + 44, 10, Color{ 160, 180, 200, 255 });
        DrawText("100ppm", x + w - 54, y + 44, 10, Color{ 160, 180, 200, 255 });
    }
    else if (landValueOverlay)
    {
        DrawText("LAND VALUE", x + 12, y + 8, 11, Color{ 46, 204, 113, 255 });
        DrawRectangleGradientH(x + 12, y + 26, w - 24, 12, Color{ 180, 40, 40, 200 }, Color{ 46, 204, 113, 255 });
        DrawText("0 Low", x + 12, y + 44, 10, Color{ 160, 180, 200, 255 });
        DrawText("100 High", x + w - 56, y + 44, 10, Color{ 160, 180, 200, 255 });
    }
    else if (housingOverlay)
    {
        DrawText("HOUSING OCCUPANCY", x + 12, y + 8, 11, Color{ 52, 152, 219, 255 });
        DrawRectangleGradientH(x + 12, y + 26, w - 24, 12, Color{ 52, 152, 219, 200 }, Color{ 231, 76, 60, 255 });
        DrawText("0% Empty", x + 12, y + 44, 10, Color{ 160, 180, 200, 255 });
        DrawText("100% Full", x + w - 58, y + 44, 10, Color{ 160, 180, 200, 255 });
    }
}

void UI::drawModeBanners(bool demolishMode, bool busStopMode, bool routeMode,
                         const std::vector<int>& currentRouteStops)
{
    const int sw = GetScreenWidth();

    if (demolishMode)
    {
        const int w = 480;
        const int h = 44;
        const int x = (sw - w) / 2;
        const int y = 56;

        DrawRectangleRounded(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                             0.25f, 4, Color{ 55, 15, 15, 240 });
        DrawRectangleRoundedLines(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                                  0.25f, 4, Color{ 231, 76, 60, 255 });

        DrawText("⚠ DEMOLISH MODE ACTIVE", x + 16, y + 8, 13, Color{ 255, 100, 100, 255 });
        DrawText("Click developed tiles to clear (₹20) • Press [D] or [Esc] to Exit", x + 16, y + 24, 11, Color{ 230, 210, 210, 255 });
    }
    else if (busStopMode)
    {
        const int w = 520;
        const int h = 44;
        const int x = (sw - w) / 2;
        const int y = 56;

        DrawRectangleRounded(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                             0.25f, 4, Color{ 15, 35, 55, 240 });
        DrawRectangleRoundedLines(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                                  0.25f, 4, Color{ 255, 215, 0, 255 });

        DrawText("🚏 BUS STOP PLACEMENT", x + 16, y + 8, 13, Color{ 255, 225, 60, 255 });
        DrawText("Click road tiles to place stop (₹500) • [Shift+Click] Remove • [B] Exit", x + 16, y + 24, 11, Color{ 210, 235, 255, 255 });
    }
    else if (routeMode)
    {
        const int w = 560;
        const int h = 54;
        const int x = (sw - w) / 2;
        const int y = 56;

        DrawRectangleRounded(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                             0.25f, 4, Color{ 45, 20, 60, 240 });
        DrawRectangleRoundedLines(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                                  0.25f, 4, Color{ 180, 50, 220, 255 });

        DrawText("🚌 BUS ROUTE BUILDER", x + 16, y + 10, 14, Color{ 255, 220, 40, 255 });
        DrawText("[ENTER] Save  [ESC] Cancel  [Shift+R] Delete Latest", x + 200, y + 11, 12, Color{ 220, 200, 240, 255 });

        std::string seq = "Stops: ";
        if (currentRouteStops.empty())
        {
            seq += "Click bus stops on road to connect sequence...";
        }
        else
        {
            for (size_t i = 0; i < currentRouteStops.size(); ++i)
            {
                if (i > 0) seq += " -> ";
                seq += "Stop #" + std::to_string(currentRouteStops[i]);
            }
            seq += " (" + std::to_string(currentRouteStops.size()) + " total)";
        }
        DrawText(seq.c_str(), x + 16, y + 30, 12, Color{ 245, 235, 255, 255 });
    }
}

void UI::drawToast(const std::string& msg, float timer, bool hasBanner)
{
    if (msg.empty() || timer <= 0.0f)
    {
        return;
    }

    const int sw = GetScreenWidth();
    const int textW = MeasureText(msg.c_str(), 14);
    const int w = textW + 40;
    const int h = 36;
    const int x = (sw - w) / 2;
    const int y = hasBanner ? 122 : 58;

    DrawRectangleRounded(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                         0.3f, 4, Color{ 20, 28, 45, 240 });
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                              0.3f, 4, Color{ 0, 180, 240, 255 });

    DrawText(msg.c_str(), x + 20, y + 11, 14, Color{ 240, 245, 255, 255 });
}

void UI::drawSelfTestModal(const SelfTest& selfTest, bool& showSelfTestModal)
{
    if (!selfTest.hasRun())
    {
        return;
    }

    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();
    const int w = 480;
    const int h = 520;
    const int x = (sw - w) / 2;
    const int y = (sh - h) / 2;

    DrawRectangle(0, 0, sw, sh, Color{ 0, 0, 0, 130 });

    DrawRectangleRounded(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                         0.08f, 4, Color{ 16, 22, 34, 250 });
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                              0.08f, 4, Color{ 60, 80, 120, 255 });

    const int passed = selfTest.getPassed();
    const int total = selfTest.getTotal();
    const bool allPassed = (total > 0 && passed == total);

    Color headerBg = allPassed ? Color{ 39, 174, 96, 255 } : Color{ 192, 57, 43, 255 };
    DrawRectangleRounded(Rectangle{ static_cast<float>(x + 12), static_cast<float>(y + 12), static_cast<float>(w - 24), 40 }, 0.2f, 4, headerBg);
    DrawText(TextFormat("SELF-TEST SUITE: %d / %d PASSED", passed, total), x + 24, y + 23, 16, WHITE);

    int listY = y + 66;
    int count = 0;
    for (const std::string& line : selfTest.getResults())
    {
        if (count >= 16)
        {
            DrawText("... (and more)", x + 24, listY, 12, Color{ 140, 160, 190, 255 });
            break;
        }

        const bool isOk = line.rfind("ok", 0) == 0;
        const bool isSkip = line.rfind("SKIP", 0) == 0;

        Color badgeColor = isOk ? Color{ 46, 204, 113, 255 } : (isSkip ? Color{ 241, 196, 15, 255 } : Color{ 231, 76, 60, 255 });
        DrawRectangleRounded(Rectangle{ static_cast<float>(x + 24), static_cast<float>(listY), 36, 18 }, 0.3f, 4, badgeColor);
        DrawText(isOk ? "PASS" : (isSkip ? "SKIP" : "FAIL"), x + 28, listY + 3, 10, Color{ 10, 20, 30, 255 });

        DrawText(line.c_str(), x + 68, listY + 2, 12, Color{ 215, 225, 240, 255 });
        listY += 24;
        ++count;
    }

    DrawRectangleRounded(Rectangle{ static_cast<float>(x + w / 2 - 60), static_cast<float>(y + h - 42), 120, 28 }, 0.3f, 4, Color{ 45, 60, 90, 255 });
    DrawText("Close (ESC)", x + w / 2 - 38, y + h - 35, 13, WHITE);

    if (IsKeyPressed(KEY_ESCAPE) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                                     GetMousePosition().y >= y + h - 42 && GetMousePosition().y <= y + h - 14 &&
                                     GetMousePosition().x >= x + w / 2 - 60 && GetMousePosition().x <= x + w / 2 + 60))
    {
        showSelfTestModal = false;
    }
}

}  // namespace urbania
