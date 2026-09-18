#include "ui/UI.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <vector>

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
    return (amount < 0 ? "-Rs. " : "Rs. ") + s;
}

UI::UI()
{
}

void UI::initialize()
{
    if (fontsLoaded)
    {
        return;
    }

    // Build codepoints list: Basic ASCII (32..126), Latin-1 Supplement (128..255)
    // plus essential symbols like bullet (•), em-dash (—), multiply (×)
    std::vector<int> codepoints;
    codepoints.reserve(512);
    for (int c = 32; c <= 255; ++c)
    {
        codepoints.push_back(c);
    }
    codepoints.push_back(0x2022); // •
    codepoints.push_back(0x2014); // —
    codepoints.push_back(0x2192); // →
    codepoints.push_back(0x00D7); // ×
    codepoints.push_back(0x00B0); // °

    const char* regPath = "assets/fonts/ui_regular.ttf";
    const char* boldPath = "assets/fonts/ui_bold.ttf";

    if (FileExists(regPath))
    {
        fontRegular = LoadFontEx(regPath, 36, codepoints.data(), static_cast<int>(codepoints.size()));
        SetTextureFilter(fontRegular.texture, TEXTURE_FILTER_BILINEAR);
    }
    if (FileExists(boldPath))
    {
        fontBold = LoadFontEx(boldPath, 36, codepoints.data(), static_cast<int>(codepoints.size()));
        SetTextureFilter(fontBold.texture, TEXTURE_FILTER_BILINEAR);
    }

    fontsLoaded = (fontRegular.texture.id > 0);
}

void UI::shutdown()
{
    if (fontRegular.texture.id > 0)
    {
        UnloadFont(fontRegular);
        fontRegular = {};
    }
    if (fontBold.texture.id > 0)
    {
        UnloadFont(fontBold);
        fontBold = {};
    }
    fontsLoaded = false;
}

void UI::drawText(const char* text, float x, float y, float fontSize, Color color, bool bold) const
{
    if (!text || text[0] == '\0') return;
    const Font& f = (bold && fontBold.texture.id > 0) ? fontBold : ((fontRegular.texture.id > 0) ? fontRegular : GetFontDefault());
    if (f.texture.id > 0 && f.texture.id != GetFontDefault().texture.id)
    {
        DrawTextEx(f, text, Vector2{ std::round(x), std::round(y) }, fontSize, 0.5f, color);
    }
    else
    {
        DrawText(text, static_cast<int>(std::round(x)), static_cast<int>(std::round(y)), static_cast<int>(std::round(fontSize)), color);
    }
}

void UI::drawTextCentered(const char* text, float centerX, float centerY, float fontSize, Color color, bool bold) const
{
    Vector2 sz = measureText(text, fontSize, bold);
    drawText(text, centerX - sz.x * 0.5f, centerY - sz.y * 0.5f, fontSize, color, bold);
}

Vector2 UI::measureText(const char* text, float fontSize, bool bold) const
{
    if (!text || text[0] == '\0') return Vector2{ 0.0f, 0.0f };
    const Font& f = (bold && fontBold.texture.id > 0) ? fontBold : ((fontRegular.texture.id > 0) ? fontRegular : GetFontDefault());
    if (f.texture.id > 0 && f.texture.id != GetFontDefault().texture.id)
    {
        return MeasureTextEx(f, text, fontSize, 0.5f);
    }
    return Vector2{ static_cast<float>(MeasureText(text, static_cast<int>(fontSize))), fontSize };
}

bool UI::isMouseOverUI() const
{
    return mouseOverUI;
}

void UI::update(SimulationClock& clock, TileType& selectedBuildType,
                bool& demolishMode, bool& busStopMode, bool& routeMode,
                bool& pollutionOverlay, bool& landValueOverlay, bool& housingOverlay,
                bool& utilitiesOverlay,
                bool& showDashboard, bool& showSelfTestModal)
{
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();
    const Vector2 m = GetMousePosition();

    mouseOverUI = false;

    // 1. Top Ribbon bounds (y: 0 to 52)
    if (m.y >= 0 && m.y <= 52)
    {
        mouseOverUI = true;

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            // Speed Controls
            // Pause (330 to 372)
            if (m.x >= 320 && m.x <= 362 && m.y >= 9 && m.y <= 41)
            {
                if (clock.isPaused()) clock.resume();
                else clock.pause();
            }
            // 1x, 2x, 4x, 8x
            const float speeds[] = { 1.0f, 2.0f, 4.0f, 8.0f };
            for (int i = 0; i < 4; ++i)
            {
                const int bx = 368 + i * 38;
                if (m.x >= bx && m.x <= bx + 34 && m.y >= 9 && m.y <= 41)
                {
                    clock.setTimeScale(speeds[i]);
                    if (clock.isPaused()) clock.resume();
                }
            }

            // Overlay Pills on top right
            int rightX = sw - 16;
            rightX -= 68; // F9 Test
            if (m.x >= rightX && m.x <= rightX + 68 && m.y >= 10 && m.y <= 40) showSelfTestModal = !showSelfTestModal;
            rightX -= (78 + 6); // TAB Dash
            if (m.x >= rightX && m.x <= rightX + 78 && m.y >= 10 && m.y <= 40) showDashboard = !showDashboard;
            rightX -= (82 + 6); // F8 Utility
            if (m.x >= rightX && m.x <= rightX + 82 && m.y >= 10 && m.y <= 40) utilitiesOverlay = !utilitiesOverlay;
            rightX -= (76 + 6); // F7 Housing
            if (m.x >= rightX && m.x <= rightX + 76 && m.y >= 10 && m.y <= 40) housingOverlay = !housingOverlay;
            rightX -= (74 + 6); // F6 Land
            if (m.x >= rightX && m.x <= rightX + 74 && m.y >= 10 && m.y <= 40) landValueOverlay = !landValueOverlay;
            rightX -= (74 + 6); // F5 Smog
            if (m.x >= rightX && m.x <= rightX + 74 && m.y >= 10 && m.y <= 40) pollutionOverlay = !pollutionOverlay;
        }
    }

    // 2. Bottom Tool Dock bounds
    const int dockW = 860;
    const int dockH = 76;
    const int dockX = (sw - dockW) / 2;
    const int dockY = sh - 88;

    if (m.x >= dockX && m.x <= dockX + dockW && m.y >= dockY && m.y <= dockY + dockH)
    {
        mouseOverUI = true;

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            const int btnW = 98;
            const int gap = 6;
            for (int i = 0; i < 8; ++i)
            {
                const int bx = dockX + 14 + i * (btnW + gap);
                const int by = dockY + 8;
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
        const int dw = 360;
        const int dh = 500;
        const int dx = sw - dw - 16;
        const int dy = 60;

        if (m.x >= dx && m.x <= dx + dw && m.y >= dy && m.y <= dy + dh)
        {
            mouseOverUI = true;

            // Tab bar clicks
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && m.y >= dy + 36 && m.y <= dy + 64)
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
              bool utilitiesOverlay,
              bool showDashboard, bool showSelfTestModal,
              const TileCoordinate& hoveredTile,
              const std::vector<int>& currentRouteStops,
              const std::string& toastMessage, float toastTimer,
              const SelfTest& selfTest)
{
    if (!fontsLoaded)
    {
        initialize();
    }

    drawTopRibbon(sim, clock, pollutionOverlay, landValueOverlay, housingOverlay,
                  utilitiesOverlay, showDashboard, showSelfTestModal);
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

    drawOverlayLegends(sim, pollutionOverlay, landValueOverlay, housingOverlay, utilitiesOverlay);
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
                       bool utilitiesOverlay,
                       bool showDashboard, bool showSelfTestModal)
{
    const int sw = GetScreenWidth();
    const int barH = 50;

    // Dark glass ribbon bar
    DrawRectangle(0, 0, sw, barH, Color{ 14, 18, 28, 248 });
    DrawRectangle(0, barH - 2, sw, 2, Color{ 38, 48, 70, 255 });

    // Logo & Brand Badge
    drawText("URBANIA", 18, 12, 22, Color{ 245, 250, 255, 255 }, true);
    DrawRectangleRounded(Rectangle{ 128, 16, 42, 18 }, 0.4f, 4, Color{ 35, 65, 115, 255 });
    drawTextCentered("CITY", 149, 25, 11, Color{ 120, 200, 255, 255 }, true);

    // Calendar & Clock Card
    DrawRectangleRounded(Rectangle{ 180, 9, 130, 32 }, 0.25f, 4, Color{ 22, 28, 44, 255 });
    DrawRectangleRoundedLines(Rectangle{ 180, 9, 130, 32 }, 0.25f, 4, Color{ 45, 58, 85, 255 });
    drawTextCentered(TextFormat("Day %d • %02d:%02d", clock.getDay(), clock.getHour(), clock.getMinute()),
                     245, 25, 14, Color{ 210, 225, 245, 255 }, true);

    // Time & Speed Controls
    const bool isPaused = clock.isPaused();
    const float speed = clock.getTimeScale();

    // Pause button
    const Color pauseBg = isPaused ? Color{ 220, 50, 50, 255 } : Color{ 26, 33, 50, 255 };
    const Color pauseText = isPaused ? WHITE : Color{ 150, 165, 190, 255 };
    DrawRectangleRounded(Rectangle{ 320, 9, 42, 32 }, 0.3f, 4, pauseBg);
    DrawRectangleRoundedLines(Rectangle{ 320, 9, 42, 32 }, 0.3f, 4, Color{ 45, 58, 85, 255 });
    drawTextCentered("||", 341, 25, 15, pauseText, true);

    // Speed buttons: 1x, 2x, 4x, 8x
    const float speeds[] = { 1.0f, 2.0f, 4.0f, 8.0f };
    const char* speedLabels[] = { "1x", "2x", "4x", "8x" };
    for (int i = 0; i < 4; ++i)
    {
        const int bx = 368 + i * 38;
        const bool active = (!isPaused && speed == speeds[i]);
        const Color btnBg = active ? Color{ 0, 185, 245, 255 } : Color{ 26, 33, 50, 255 };
        const Color btnText = active ? Color{ 10, 20, 30, 255 } : Color{ 150, 165, 190, 255 };
        DrawRectangleRounded(Rectangle{ static_cast<float>(bx), 9, 34, 32 }, 0.3f, 4, btnBg);
        DrawRectangleRoundedLines(Rectangle{ static_cast<float>(bx), 9, 34, 32 }, 0.3f, 4, Color{ 45, 58, 85, 255 });
        drawTextCentered(speedLabels[i], bx + 17, 25, 13, btnText, true);
    }

    // Money & Daily Cash Flow (Center)
    const int moneyX = 535;
    DrawRectangleRounded(Rectangle{ static_cast<float>(moneyX), 9, 180, 32 }, 0.25f, 4, Color{ 22, 28, 44, 255 });
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(moneyX), 9, 180, 32 }, 0.25f, 4, Color{ 45, 58, 85, 255 });
    drawText(formatRupees(sim.getEconomy().getMoney()).c_str(), moneyX + 10, 16, 15, Color{ 46, 204, 113, 255 }, true);

    const int netInc = static_cast<int>(sim.getEconomy().getNetIncome());
    if (netInc >= 0)
    {
        drawText(TextFormat("+%s/d", formatRupees(netInc).c_str()), moneyX + 110, 17, 12, Color{ 46, 204, 113, 220 });
    }
    else
    {
        drawText(TextFormat("-%s/d", formatRupees(-netInc).c_str()), moneyX + 110, 17, 12, Color{ 231, 76, 60, 220 });
    }

    // Population & Happiness
    const int popX = 725;
    DrawRectangleRounded(Rectangle{ static_cast<float>(popX), 9, 215, 32 }, 0.25f, 4, Color{ 22, 28, 44, 255 });
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(popX), 9, 215, 32 }, 0.25f, 4, Color{ 45, 58, 85, 255 });
    drawText(TextFormat("Pop: %d", sim.getPopulation().getTotalPopulation()), popX + 10, 16, 14, WHITE, true);

    const float happy = sim.getHappiness().getAverageHappiness();
    Color happyCol = Color{ 46, 204, 113, 255 }; // Green
    if (happy < 40.0f) happyCol = Color{ 231, 76, 60, 255 }; // Red
    else if (happy < 65.0f) happyCol = Color{ 241, 196, 15, 255 }; // Yellow

    DrawRectangleRounded(Rectangle{ static_cast<float>(popX + 98), 12, 108, 26 }, 0.3f, 4,
                         Color{ happyCol.r, happyCol.g, happyCol.b, 40 });
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(popX + 98), 12, 108, 26 }, 0.3f, 4, happyCol);
    drawTextCentered(TextFormat("%.1f%% Happy", happy), popX + 152, 25, 12, happyCol, true);

    // Right-side Overlay & Feature Pills (dynamically laid out from right margin)
    int rx = sw - 16;
    const int pillY = 10;
    const int pillH = 30;

    // F9 Test
    rx -= 68;
    const Color testBg = showSelfTestModal ? Color{ 230, 126, 34, 255 } : Color{ 26, 33, 50, 255 };
    const Color testText = showSelfTestModal ? WHITE : Color{ 150, 165, 190, 255 };
    DrawRectangleRounded(Rectangle{ static_cast<float>(rx), static_cast<float>(pillY), 68, static_cast<float>(pillH) }, 0.3f, 4, testBg);
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(rx), static_cast<float>(pillY), 68, static_cast<float>(pillH) }, 0.3f, 4, Color{ 45, 58, 85, 255 });
    drawTextCentered("F9 Test", rx + 34, pillY + 15, 12, testText, true);

    // TAB Dash
    rx -= (78 + 6);
    const Color dashBg = showDashboard ? Color{ 142, 68, 173, 255 } : Color{ 26, 33, 50, 255 };
    const Color dashText = showDashboard ? WHITE : Color{ 150, 165, 190, 255 };
    DrawRectangleRounded(Rectangle{ static_cast<float>(rx), static_cast<float>(pillY), 78, static_cast<float>(pillH) }, 0.3f, 4, dashBg);
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(rx), static_cast<float>(pillY), 78, static_cast<float>(pillH) }, 0.3f, 4, Color{ 45, 58, 85, 255 });
    drawTextCentered("TAB Dash", rx + 39, pillY + 15, 12, dashText, true);

    // F8 Utility
    rx -= (82 + 6);
    const Color utilBg = utilitiesOverlay ? Color{ 0, 180, 240, 255 } : Color{ 26, 33, 50, 255 };
    const Color utilText = utilitiesOverlay ? Color{ 10, 20, 30, 255 } : Color{ 150, 165, 190, 255 };
    DrawRectangleRounded(Rectangle{ static_cast<float>(rx), static_cast<float>(pillY), 82, static_cast<float>(pillH) }, 0.3f, 4, utilBg);
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(rx), static_cast<float>(pillY), 82, static_cast<float>(pillH) }, 0.3f, 4, Color{ 45, 58, 85, 255 });
    drawTextCentered("F8 Utility", rx + 41, pillY + 15, 12, utilText, true);

    // F7 Housing
    rx -= (76 + 6);
    const Color houseBg = housingOverlay ? Color{ 52, 152, 219, 255 } : Color{ 26, 33, 50, 255 };
    const Color houseText = housingOverlay ? WHITE : Color{ 150, 165, 190, 255 };
    DrawRectangleRounded(Rectangle{ static_cast<float>(rx), static_cast<float>(pillY), 76, static_cast<float>(pillH) }, 0.3f, 4, houseBg);
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(rx), static_cast<float>(pillY), 76, static_cast<float>(pillH) }, 0.3f, 4, Color{ 45, 58, 85, 255 });
    drawTextCentered("F7 House", rx + 38, pillY + 15, 12, houseText, true);

    // F6 Land Value
    rx -= (74 + 6);
    const Color landBg = landValueOverlay ? Color{ 46, 204, 113, 255 } : Color{ 26, 33, 50, 255 };
    const Color landText = landValueOverlay ? Color{ 10, 25, 15, 255 } : Color{ 150, 165, 190, 255 };
    DrawRectangleRounded(Rectangle{ static_cast<float>(rx), static_cast<float>(pillY), 74, static_cast<float>(pillH) }, 0.3f, 4, landBg);
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(rx), static_cast<float>(pillY), 74, static_cast<float>(pillH) }, 0.3f, 4, Color{ 45, 58, 85, 255 });
    drawTextCentered("F6 Land", rx + 37, pillY + 15, 12, landText, true);

    // F5 Smog
    rx -= (74 + 6);
    const Color smogBg = pollutionOverlay ? Color{ 210, 105, 30, 255 } : Color{ 26, 33, 50, 255 };
    const Color smogText = pollutionOverlay ? WHITE : Color{ 150, 165, 190, 255 };
    DrawRectangleRounded(Rectangle{ static_cast<float>(rx), static_cast<float>(pillY), 74, static_cast<float>(pillH) }, 0.3f, 4, smogBg);
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(rx), static_cast<float>(pillY), 74, static_cast<float>(pillH) }, 0.3f, 4, Color{ 45, 58, 85, 255 });
    drawTextCentered("F5 Smog", rx + 37, pillY + 15, 12, smogText, true);
}

void UI::drawDemandMeters(const Simulation& sim)
{
    const int x = 16;
    const int y = 62;
    const int w = 154;
    const int h = 170;

    DrawRectangleRounded(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                         0.1f, 4, Color{ 14, 18, 28, 235 });
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                              0.1f, 4, Color{ 40, 52, 75, 255 });

    drawTextCentered("RCI DEMAND", x + w / 2, y + 16, 13, Color{ 200, 215, 235, 255 }, true);
    DrawLine(x + 12, y + 28, x + w - 12, y + 28, Color{ 40, 52, 75, 255 });

    const int baseY = y + 96;
    const int colW = 28;
    const int maxBarH = 46;

    DrawLine(x + 16, baseY, x + w - 16, baseY, Color{ 70, 85, 115, 255 });

    // Residential (R)
    const int rDem = sim.getDemand().getResidentialDemand();
    const int rX = x + 24;
    const float rFrac = std::clamp(static_cast<float>(rDem) / 100.0f, -1.0f, 1.0f);
    const int rH = static_cast<int>(std::abs(rFrac) * maxBarH);
    if (rFrac >= 0) DrawRectangle(rX, baseY - rH, colW, rH, Color{ 46, 204, 113, 230 });
    else DrawRectangle(rX, baseY, colW, rH, Color{ 39, 174, 96, 120 });
    DrawRectangleLines(rX, baseY - maxBarH, colW, maxBarH * 2, Color{ 46, 204, 113, 80 });
    drawTextCentered("R", rX + colW / 2, baseY + maxBarH + 12, 13, Color{ 46, 204, 113, 255 }, true);
    drawTextCentered(TextFormat("%+d", rDem), rX + colW / 2, (rFrac >= 0 ? baseY - rH - 8 : baseY + rH + 8), 11, Color{ 210, 235, 220, 255 });

    // Commercial (C)
    const int cDem = sim.getDemand().getCommercialDemand();
    const int cX = x + 64;
    const float cFrac = std::clamp(static_cast<float>(cDem) / 100.0f, -1.0f, 1.0f);
    const int cH = static_cast<int>(std::abs(cFrac) * maxBarH);
    if (cFrac >= 0) DrawRectangle(cX, baseY - cH, colW, cH, Color{ 52, 152, 219, 230 });
    else DrawRectangle(cX, baseY, colW, cH, Color{ 41, 128, 185, 120 });
    DrawRectangleLines(cX, baseY - maxBarH, colW, maxBarH * 2, Color{ 52, 152, 219, 80 });
    drawTextCentered("C", cX + colW / 2, baseY + maxBarH + 12, 13, Color{ 52, 152, 219, 255 }, true);
    drawTextCentered(TextFormat("%+d", cDem), cX + colW / 2, (cFrac >= 0 ? baseY - cH - 8 : baseY + cH + 8), 11, Color{ 210, 230, 255, 255 });

    // Industrial (I)
    const int iDem = sim.getDemand().getIndustrialDemand();
    const int iX = x + 104;
    const float iFrac = std::clamp(static_cast<float>(iDem) / 100.0f, -1.0f, 1.0f);
    const int iH = static_cast<int>(std::abs(iFrac) * maxBarH);
    if (iFrac >= 0) DrawRectangle(iX, baseY - iH, colW, iH, Color{ 230, 126, 34, 230 });
    else DrawRectangle(iX, baseY, colW, iH, Color{ 211, 84, 0, 120 });
    DrawRectangleLines(iX, baseY - maxBarH, colW, maxBarH * 2, Color{ 230, 126, 34, 80 });
    drawTextCentered("I", iX + colW / 2, baseY + maxBarH + 12, 13, Color{ 230, 126, 34, 255 }, true);
    drawTextCentered(TextFormat("%+d", iDem), iX + colW / 2, (iFrac >= 0 ? baseY - iH - 8 : baseY + iH + 8), 11, Color{ 255, 230, 210, 255 });
}

void UI::drawBottomDock(const Simulation& sim, TileType selectedBuildType,
                        bool demolishMode, bool busStopMode, bool routeMode)
{
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();
    const int dockW = 860;
    const int dockH = 76;
    const int dockX = (sw - dockW) / 2;
    const int dockY = sh - 88;

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
        { "[1]", "Road", "Rs. 100", "Lays road pavement for citizen commutes & bus lines", Color{ 130, 135, 145, 255 },
          !demolishMode && !busStopMode && !routeMode && selectedBuildType == TileType::Road,
          sim.getEconomy().canAfford(TileType::Road) },
        { "[2]", "Resi", "Rs. 2,000", "Zones residential plots for citizens to build homes", Color{ 70, 130, 220, 255 },
          !demolishMode && !busStopMode && !routeMode && selectedBuildType == TileType::Residential,
          sim.getEconomy().canAfford(TileType::Residential) },
        { "[3]", "Comm", "Rs. 5,000", "Zones commercial services and shops for city revenue", Color{ 240, 160, 40, 255 },
          !demolishMode && !busStopMode && !routeMode && selectedBuildType == TileType::Commercial,
          sim.getEconomy().canAfford(TileType::Commercial) },
        { "[4]", "Ind", "Rs. 10,000", "Zones factories creating jobs (generates heavy smog)", Color{ 175, 75, 75, 255 },
          !demolishMode && !busStopMode && !routeMode && selectedBuildType == TileType::Industrial,
          sim.getEconomy().canAfford(TileType::Industrial) },
        { "[5]", "Park", "Rs. 1,000", "Plants city parks (+15 land value boost, filters smog)", Color{ 35, 140, 60, 255 },
          !demolishMode && !busStopMode && !routeMode && selectedBuildType == TileType::Park,
          sim.getEconomy().canAfford(TileType::Park) },
        { "[B]", "Bus Stop", "Rs. 500", "Installs transit stops on roads for bus routes", Color{ 255, 215, 0, 255 },
          busStopMode,
          sim.getEconomy().canAfford(urbania::Transit::BUS_STOP_COST) },
        { "[R]", "Route", "Transit", "Connects placed bus stops into active commuter routes", Color{ 180, 50, 220, 255 },
          routeMode,
          true },
        { "[D]", "Demolish", "Free", "Clears structures and roads back to open grass", Color{ 220, 50, 50, 255 },
          demolishMode,
          true }
    };

    const int btnW = 98;
    const int btnH = 60;
    const int gap = 6;
    const Vector2 mouse = GetMousePosition();
    int hoveredIdx = -1;

    for (int i = 0; i < 8; ++i)
    {
        const int bx = dockX + 14 + i * (btnW + gap);
        const int by = dockY + 8;
        const bool hovered = (mouse.x >= bx && mouse.x <= bx + btnW && mouse.y >= by && mouse.y <= by + btnH);
        const bool selected = tools[i].active;
        if (hovered) hoveredIdx = i;

        Color btnBg = selected ? Color{ 35, 52, 85, 255 } : (hovered ? Color{ 25, 34, 52, 255 } : Color{ 18, 24, 38, 255 });
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
        drawText(tools[i].key, bx + 30, by + 8, 12, Color{ 130, 155, 195, 255 }, true);

        // Name
        Color nameCol = selected ? WHITE : (tools[i].affordable ? Color{ 210, 220, 235, 255 } : Color{ 130, 140, 155, 255 });
        drawText(tools[i].name, bx + 8, by + 28, 13, nameCol, true);

        // Price
        Color costCol = tools[i].affordable ? Color{ 46, 204, 113, 220 } : Color{ 231, 76, 60, 240 };
        drawText(tools[i].cost, bx + 8, by + 43, 11, costCol);
    }

    // Floating Tooltip above dock when hovered
    if (hoveredIdx >= 0)
    {
        const auto& t = tools[hoveredIdx];
        std::string tipText = std::string(t.name) + " " + t.key + " • " + t.cost;
        if (!t.affordable) tipText += " (Not Enough Funds)";
        tipText += " — " + std::string(t.desc);

        Vector2 sz = measureText(tipText.c_str(), 13, false);
        const int tipW = static_cast<int>(sz.x) + 28;
        const int tipH = 28;
        const int tipX = std::clamp(static_cast<int>(mouse.x) - tipW / 2, 16, sw - tipW - 16);
        const int tipY = dockY - tipH - 8;

        DrawRectangleRounded(Rectangle{ static_cast<float>(tipX), static_cast<float>(tipY),
                                       static_cast<float>(tipW), static_cast<float>(tipH) },
                             0.3f, 4, Color{ 10, 14, 22, 245 });
        DrawRectangleRoundedLines(Rectangle{ static_cast<float>(tipX), static_cast<float>(tipY),
                                            static_cast<float>(tipW), static_cast<float>(tipH) },
                                  0.3f, 4, Color{ 60, 80, 115, 255 });
        drawTextCentered(tipText.c_str(), tipX + tipW / 2, tipY + tipH / 2, 13, t.affordable ? Color{ 230, 240, 255, 255 } : Color{ 255, 170, 170, 255 });
    }
}

void UI::drawDashboard(const World& world, const Simulation& sim)
{
    (void)world;
    const int sw = GetScreenWidth();
    const int w = 360;
    const int h = 500;
    const int x = sw - w - 16;
    const int y = 60;

    DrawRectangleRounded(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                         0.05f, 4, Color{ 14, 18, 28, 248 });
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                              0.05f, 4, Color{ 40, 52, 75, 255 });

    // Header
    drawText("CITY DASHBOARD", x + 16, y + 12, 15, Color{ 200, 215, 240, 255 }, true);
    drawText("[TAB to Close]", x + w - 100, y + 14, 12, Color{ 120, 140, 175, 255 });

    // Tab switcher
    const char* tabNames[5] = { "Overview", "Economy", "People", "Transit", "Eco" };
    const int tabW = (w - 24) / 5;
    for (int t = 0; t < 5; ++t)
    {
        const int tx = x + 12 + t * tabW;
        const int ty = y + 36;
        const bool active = (static_cast<int>(currentTab) == t);
        const Color tabBg = active ? Color{ 35, 65, 115, 255 } : Color{ 22, 28, 42, 255 };
        const Color tabTxt = active ? WHITE : Color{ 140, 155, 180, 255 };

        DrawRectangleRounded(Rectangle{ static_cast<float>(tx), static_cast<float>(ty), static_cast<float>(tabW - 2), 26 }, 0.2f, 2, tabBg);
        drawTextCentered(tabNames[t], tx + (tabW - 2) / 2, ty + 13, 12, tabTxt, active);
    }

    DrawLine(x + 12, y + 68, x + w - 12, y + 68, Color{ 35, 48, 70, 255 });

    int cy = y + 78;

    switch (currentTab)
    {
    case DashboardTab::Overview:
    {
        drawText("CITY OVERVIEW", x + 16, cy, 13, Color{ 0, 180, 240, 255 }, true);
        cy += 22;
        drawText(TextFormat("Total Population: %d citizens", sim.getPopulation().getTotalPopulation()), x + 16, cy, 13, WHITE);
        cy += 20;
        drawText(TextFormat("Housing Capacity: %d / %d (%.0f%% full)", sim.getHousing().getTotalResidents(), sim.getHousing().getTotalCapacity(), sim.getHousing().getOccupancyRatio() * 100.0f), x + 16, cy, 13, Color{ 190, 205, 225, 255 });
        cy += 20;
        drawText(TextFormat("Average Happiness: %.1f%%", sim.getHappiness().getAverageHappiness()), x + 16, cy, 13, Color{ 46, 204, 113, 255 });
        cy += 20;
        drawText(TextFormat("Average Land Value: %.1f / 100", sim.getLandValue().getAverageLandValue()), x + 16, cy, 13, Color{ 190, 205, 225, 255 });
        cy += 20;
        drawText(TextFormat("Average Pollution: %.1f ppm", sim.getPollution().getAveragePollution()), x + 16, cy, 13, Color{ 230, 126, 34, 255 });
        cy += 26;

        drawText("MUNICIPAL UTILITIES", x + 16, cy, 13, Color{ 0, 180, 240, 255 }, true);
        cy += 20;
        drawText(TextFormat("Power: %d / %d kW • Water: %d / %d kL",
                            sim.getUtilities().getElectricityDemand(), sim.getUtilities().getElectricityCapacity(),
                            sim.getUtilities().getWaterDemand(), sim.getUtilities().getWaterCapacity()),
                 x + 16, cy, 13, WHITE);
        cy += 20;
        drawText(TextFormat("Sewage: %d / %d kL • Supplied: %d / %d",
                            sim.getUtilities().getSewageDemand(), sim.getUtilities().getSewageCapacity(),
                            sim.getUtilities().getSuppliedBuildingCount(), sim.getUtilities().getTotalDevelopedBuildingCount()),
                 x + 16, cy, 13, Color{ 190, 205, 225, 255 });
        cy += 26;

        const int net = static_cast<int>(sim.getEconomy().getNetIncome());
        drawText("DAILY CASH FLOW", x + 16, cy, 13, Color{ 0, 180, 240, 255 }, true);
        cy += 20;
        drawText(TextFormat("Net Daily Margin: %s%s / day", (net >= 0 ? "+" : "-"), formatRupees(std::abs(net)).c_str()),
                 x + 16, cy, 14, (net >= 0 ? Color{ 46, 204, 113, 255 } : Color{ 231, 76, 60, 255 }), true);
        break;
    }

    case DashboardTab::Economy:
    {
        drawText("FINANCES & LEDGER", x + 16, cy, 13, Color{ 46, 204, 113, 255 }, true);
        cy += 22;
        drawText(TextFormat("Treasury Balance: %s", formatRupees(sim.getEconomy().getMoney()).c_str()), x + 16, cy, 13, WHITE, true);
        cy += 20;
        drawText(TextFormat("Daily Tax Revenue: +%s", formatRupees(static_cast<int>(sim.getEconomy().getTaxIncome())).c_str()), x + 16, cy, 13, Color{ 46, 204, 113, 255 });
        cy += 20;
        drawText(TextFormat("Municipal Upkeep: -%s", formatRupees(static_cast<int>(sim.getEconomy().getMaintenanceCost())).c_str()), x + 16, cy, 13, Color{ 231, 76, 60, 255 });
        cy += 18;
        drawText(TextFormat(" (Includes Utility Upkeep: -%s)", formatRupees(static_cast<int>(sim.getEconomy().getUtilityMaintenanceCost())).c_str()), x + 16, cy, 12, Color{ 160, 180, 210, 255 });
        cy += 24;

        const int net = static_cast<int>(sim.getEconomy().getNetIncome());
        drawText(TextFormat("Net Daily Margin: %s%s / day", (net >= 0 ? "+" : "-"), formatRupees(std::abs(net)).c_str()),
                 x + 16, cy, 14, (net >= 0 ? Color{ 46, 204, 113, 255 } : Color{ 231, 76, 60, 255 }), true);
        cy += 30;

        drawText("ZONE DEMAND INDICES", x + 16, cy, 13, Color{ 0, 180, 240, 255 }, true);
        cy += 22;
        drawText(TextFormat("Residential Demand: %+d", sim.getDemand().getResidentialDemand()), x + 16, cy, 13, Color{ 46, 204, 113, 255 });
        cy += 20;
        drawText(TextFormat("Commercial Demand: %+d", sim.getDemand().getCommercialDemand()), x + 16, cy, 13, Color{ 52, 152, 219, 255 });
        cy += 20;
        drawText(TextFormat("Industrial Demand: %+d", sim.getDemand().getIndustrialDemand()), x + 16, cy, 13, Color{ 230, 126, 34, 255 });
        break;
    }

    case DashboardTab::Population:
    {
        drawText("CITIZENS & WORKFORCE", x + 16, cy, 13, Color{ 52, 152, 219, 255 }, true);
        cy += 22;
        drawText(TextFormat("Total Citizens: %d", sim.getPopulation().getTotalPopulation()), x + 16, cy, 13, WHITE, true);
        cy += 20;
        drawText(TextFormat("Employed: %d • Unemployed: %d", sim.getEmployment().getEmployedCitizens(), sim.getEmployment().getUnemployedCitizens()), x + 16, cy, 13, Color{ 190, 205, 225, 255 });
        cy += 20;
        drawText(TextFormat("Workplace Jobs: %d / %d filled", sim.getEmployment().getOccupiedJobs(), sim.getEmployment().getTotalJobs()), x + 16, cy, 13, Color{ 190, 205, 225, 255 });
        cy += 28;

        drawText("RESIDENTIAL CAPACITY", x + 16, cy, 13, Color{ 0, 180, 240, 255 }, true);
        cy += 22;
        drawText(TextFormat("Capacity: %d beds • %d residents", sim.getHousing().getTotalCapacity(), sim.getHousing().getTotalResidents()), x + 16, cy, 13, Color{ 190, 205, 225, 255 });
        cy += 20;
        drawText(TextFormat("Housing Pressure: %+d", sim.getHousing().getHousingPressure()), x + 16, cy, 13, (sim.getHousing().getHousingPressure() > 0 ? Color{ 241, 196, 15, 255 } : Color{ 140, 160, 190, 255 }));
        break;
    }

    case DashboardTab::Transit:
    {
        drawText("MOBILITY & TRANSIT", x + 16, cy, 13, Color{ 230, 126, 34, 255 }, true);
        cy += 22;
        drawText(TextFormat("Road Network: %d road nodes", sim.getRoadNetwork().getNodeCount()), x + 16, cy, 13, WHITE);
        cy += 20;
        drawText(TextFormat("Private Vehicles: %d active cars", sim.getTraffic().getActiveVehicleCount()), x + 16, cy, 13, Color{ 190, 205, 225, 255 });
        cy += 20;
        drawText(TextFormat("Congested Roads: %d (Max: %.1fx)", sim.getCongestion().getCongestedRoadCount(), sim.getCongestion().getMaxCongestion()), x + 16, cy, 13, Color{ 190, 205, 225, 255 });
        cy += 28;

        drawText("PUBLIC BUS TRANSIT", x + 16, cy, 13, Color{ 255, 215, 0, 255 }, true);
        cy += 22;
        drawText(TextFormat("Bus Stops: %d placed", sim.getTransit().getBusStopCount()), x + 16, cy, 13, Color{ 190, 205, 225, 255 });
        cy += 20;
        drawText(TextFormat("Bus Routes: %d configured", sim.getTransit().getRouteCount()), x + 16, cy, 13, Color{ 190, 205, 225, 255 });
        cy += 20;
        drawText(TextFormat("Buses: %d active / %d total", sim.getTransit().getActiveBusCount(), sim.getTransit().getBusCount()), x + 16, cy, 13, Color{ 255, 215, 0, 255 }, true);
        cy += 20;
        drawText(TextFormat("Commuters: %d routed • %d unrouted", sim.getCommuteSystem().getRoutedCitizens(), sim.getCommuteSystem().getUnroutedCitizens()), x + 16, cy, 13, Color{ 190, 205, 225, 255 });
        break;
    }

    case DashboardTab::Environment:
    {
        drawText("ENVIRONMENT & LAND VALUE", x + 16, cy, 13, Color{ 46, 204, 113, 255 }, true);
        cy += 22;
        drawText(TextFormat("Average Pollution: %.1f ppm", sim.getPollution().getAveragePollution()), x + 16, cy, 13, Color{ 230, 126, 34, 255 });
        cy += 20;
        drawText(TextFormat("Max Pollution Hotspot: %.1f ppm", sim.getPollution().getMaxPollution()), x + 16, cy, 13, Color{ 231, 76, 60, 255 });
        cy += 20;
        drawText(TextFormat("Average Land Desirability: %.1f / 100", sim.getLandValue().getAverageLandValue()), x + 16, cy, 13, Color{ 46, 204, 113, 255 });
        cy += 28;

        drawText("HAPPINESS IMPACT FACTORS", x + 16, cy, 13, Color{ 0, 180, 240, 255 }, true);
        cy += 22;
        drawText("• Industrial zones create smog plumes", x + 16, cy, 12, Color{ 190, 205, 225, 255 });
        cy += 20;
        drawText("• Parks clean smog & boost nearby Land Value", x + 16, cy, 12, Color{ 190, 205, 225, 255 });
        cy += 20;
        drawText("• Heavy traffic congestion slows commutes", x + 16, cy, 12, Color{ 190, 205, 225, 255 });
        cy += 20;
        drawText("• Unsupplied homes incur -20 happiness penalty", x + 16, cy, 12, Color{ 241, 196, 15, 255 });
        break;
    }
    }
}

void UI::drawTileInspector(const World& world, const Simulation& sim,
                           const TileCoordinate& hovered)
{
    const int sw = GetScreenWidth();
    const int w = 270;
    const int h = 195;
    const int x = sw - w - 16;
    const int y = GetScreenHeight() - h - 94;

    DrawRectangleRounded(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                         0.1f, 4, Color{ 14, 18, 28, 245 });
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                              0.1f, 4, Color{ 40, 52, 75, 255 });

    const Tile& t = world.getTile(hovered.x, hovered.y);
    int cy = y + 10;

    // Header with Zone Badge
    drawText(TextFormat("TILE (%d, %d)", hovered.x, hovered.y), x + 12, cy, 14, WHITE, true);

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

    DrawRectangleRounded(Rectangle{ static_cast<float>(x + 148), static_cast<float>(cy - 1), 110, 20 }, 0.3f, 4, badgeCol);
    drawTextCentered(typeName, x + 203, cy + 9, 12, Color{ 10, 20, 30, 255 }, true);
    cy += 24;

    DrawLine(x + 10, cy, x + w - 10, cy, Color{ 35, 48, 70, 255 });
    cy += 10;

    // Land Value
    const float lv = sim.getLandValue().getLandValue(hovered.x, hovered.y);
    drawText(TextFormat("Land Value: %.1f / 100", lv), x + 12, cy, 13, Color{ 190, 205, 225, 255 });
    cy += 20;

    // Specific tile metrics
    if (t.type == TileType::Residential)
    {
        const int res = sim.getPopulation().getResidentsAt(hovered.x, hovered.y);
        drawText(TextFormat("Residents: %d / %d", res, Housing::CAPACITY_PER_TILE), x + 12, cy, 13, Color{ 52, 152, 219, 255 });
        cy += 20;
    }
    else if (t.type == TileType::Commercial)
    {
        drawText("Commercial Shop: Active", x + 12, cy, 13, Color{ 240, 160, 40, 255 });
        cy += 20;
    }
    else if (t.type == TileType::Industrial)
    {
        const float p = sim.getPollution().getPollution(hovered.x, hovered.y);
        drawText(TextFormat("Smog Output: %.1f ppm", p), x + 12, cy, 13, Color{ 230, 126, 34, 255 });
        cy += 20;
    }
    else if (t.type == TileType::Road)
    {
        const int vCount = sim.getCongestion().getVehicleCount(hovered.x, hovered.y);
        const float cong = sim.getCongestion().getCongestion(hovered.x, hovered.y);
        drawText(TextFormat("Traffic: %d / 5 (%.1fx delay)", vCount, cong), x + 12, cy, 13, (cong > 1.0f ? Color{ 231, 76, 60, 255 } : Color{ 190, 205, 225, 255 }));
        cy += 20;
    }
    else if (t.type == TileType::Park)
    {
        drawText("Park: +15 Land Value Bonus", x + 12, cy, 13, Color{ 46, 204, 113, 255 });
        cy += 20;
    }

    if (t.type == TileType::Residential || t.type == TileType::Commercial || t.type == TileType::Industrial)
    {
        const auto uStatus = sim.getUtilities().getTileStatus(hovered);
        if (uStatus.isFullySupplied)
        {
            drawText("Utilities: Supplied (Power/Water/Sewage)", x + 12, cy, 12, Color{ 0, 200, 240, 255 });
            cy += 20;
        }
        else
        {
            std::string s = "Utilities: Unsupplied (";
            if (!uStatus.connected) s += "No Road";
            else {
                if (!uStatus.hasElectricity) s += "No Power ";
                if (!uStatus.hasWater) s += "No Water ";
                if (!uStatus.hasSewage) s += "No Sewage";
            }
            s += ")";
            drawText(s.c_str(), x + 12, cy, 12, Color{ 231, 76, 60, 255 });
            cy += 20;
        }
    }

    if (sim.getTransit().hasBusStop(hovered))
    {
        const auto* stop = sim.getTransit().getBusStop(hovered);
        if (stop != nullptr)
        {
            drawText(TextFormat("Transit: Bus Stop #%d", stop->id), x + 12, cy, 13, Color{ 255, 215, 0, 255 }, true);
            cy += 20;
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
                drawText(TextFormat("Bus #%d • Route #%d (Stop %d/%d)", bus.id, bus.routeId,
                                    bus.currentStopIndex + 1, total),
                         x + 12, cy, 12, Color{ 255, 215, 0, 255 });
                cy += 20;
                break;
            }
        }
    }

    const float p = sim.getPollution().getPollution(hovered.x, hovered.y);
    if (p > 0.01f)
    {
        drawText(TextFormat("Air Pollution: %.1f ppm", p), x + 12, cy, 13, Color{ 230, 126, 34, 255 });
    }
}

void UI::drawOverlayLegends(const Simulation& sim, bool pollutionOverlay, bool landValueOverlay,
                            bool housingOverlay, bool utilitiesOverlay)
{
    if (!pollutionOverlay && !landValueOverlay && !housingOverlay && !utilitiesOverlay)
    {
        return;
    }

    const int x = 16;
    const int y = 242;
    const int w = 154;
    const int h = utilitiesOverlay ? 90 : 80;

    DrawRectangleRounded(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                         0.1f, 4, Color{ 14, 18, 28, 235 });
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                              0.1f, 4, Color{ 40, 52, 75, 255 });

    if (utilitiesOverlay)
    {
        drawText("UTILITIES (F8)", x + 12, y + 8, 12, Color{ 0, 200, 240, 255 }, true);
        DrawRectangle(x + 12, y + 26, 12, 12, Color{ 0, 200, 240, 200 });
        drawText("Supplied", x + 28, y + 26, 11, Color{ 210, 235, 255, 255 });
        DrawRectangle(x + 82, y + 26, 12, 12, Color{ 231, 76, 60, 200 });
        drawText("Unsupplied", x + 98, y + 26, 11, Color{ 255, 210, 210, 255 });
        drawText(TextFormat("Demand: %dE / %dW / %dS", sim.getUtilities().getElectricityDemand(),
                            sim.getUtilities().getWaterDemand(), sim.getUtilities().getSewageDemand()),
                 x + 12, y + 46, 11, Color{ 160, 185, 215, 255 });
        drawText(TextFormat("Capacity: %d / %d / %d", sim.getUtilities().getElectricityCapacity(),
                            sim.getUtilities().getWaterCapacity(), sim.getUtilities().getSewageCapacity()),
                 x + 12, y + 66, 11, Color{ 130, 160, 195, 255 });
    }
    else if (pollutionOverlay)
    {
        drawText("POLLUTION SMOG", x + 12, y + 8, 12, Color{ 230, 126, 34, 255 }, true);
        DrawRectangleGradientH(x + 12, y + 26, w - 24, 12, Color{ 60, 140, 80, 200 }, Color{ 180, 40, 40, 255 });
        drawText("0 Clean", x + 12, y + 46, 11, Color{ 160, 180, 200, 255 });
        drawText("100ppm", x + w - 56, y + 46, 11, Color{ 160, 180, 200, 255 });
    }
    else if (landValueOverlay)
    {
        drawText("LAND VALUE", x + 12, y + 8, 12, Color{ 46, 204, 113, 255 }, true);
        DrawRectangleGradientH(x + 12, y + 26, w - 24, 12, Color{ 180, 40, 40, 200 }, Color{ 46, 204, 113, 255 });
        drawText("0 Low", x + 12, y + 46, 11, Color{ 160, 180, 200, 255 });
        drawText("100 High", x + w - 58, y + 46, 11, Color{ 160, 180, 200, 255 });
    }
    else if (housingOverlay)
    {
        drawText("HOUSING OCCUPANCY", x + 12, y + 8, 12, Color{ 52, 152, 219, 255 }, true);
        DrawRectangleGradientH(x + 12, y + 26, w - 24, 12, Color{ 52, 152, 219, 200 }, Color{ 231, 76, 60, 255 });
        drawText("0% Empty", x + 12, y + 46, 11, Color{ 160, 180, 200, 255 });
        drawText("100% Full", x + w - 60, y + 46, 11, Color{ 160, 180, 200, 255 });
    }
}

void UI::drawModeBanners(bool demolishMode, bool busStopMode, bool routeMode,
                         const std::vector<int>& currentRouteStops)
{
    const int sw = GetScreenWidth();

    if (demolishMode)
    {
        const int w = 500;
        const int h = 48;
        const int x = (sw - w) / 2;
        const int y = 60;

        DrawRectangleRounded(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                             0.25f, 4, Color{ 55, 15, 15, 240 });
        DrawRectangleRoundedLines(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                                  0.25f, 4, Color{ 231, 76, 60, 255 });

        drawText("DEMOLISH MODE ACTIVE", x + 16, y + 8, 14, Color{ 255, 100, 100, 255 }, true);
        drawText("Click developed tiles to clear (Free) • Press [D] or [Esc] to Exit", x + 16, y + 26, 12, Color{ 230, 210, 210, 255 });
    }
    else if (busStopMode)
    {
        const int w = 540;
        const int h = 48;
        const int x = (sw - w) / 2;
        const int y = 60;

        DrawRectangleRounded(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                             0.25f, 4, Color{ 15, 35, 55, 240 });
        DrawRectangleRoundedLines(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                                  0.25f, 4, Color{ 255, 215, 0, 255 });

        drawText("BUS STOP PLACEMENT", x + 16, y + 8, 14, Color{ 255, 225, 60, 255 }, true);
        drawText("Click road tiles to place stop (Rs. 500) • [Shift+Click] Remove • [B] Exit", x + 16, y + 26, 12, Color{ 210, 235, 255, 255 });
    }
    else if (routeMode)
    {
        const int w = 580;
        const int h = 58;
        const int x = (sw - w) / 2;
        const int y = 60;

        DrawRectangleRounded(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                             0.25f, 4, Color{ 45, 20, 60, 240 });
        DrawRectangleRoundedLines(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                                  0.25f, 4, Color{ 180, 50, 220, 255 });

        drawText("BUS ROUTE BUILDER", x + 16, y + 10, 15, Color{ 255, 220, 40, 255 }, true);
        drawText("[ENTER] Save  [ESC] Cancel  [Shift+R] Delete Latest", x + 200, y + 11, 13, Color{ 220, 200, 240, 255 });

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
        drawText(seq.c_str(), x + 16, y + 32, 13, Color{ 245, 235, 255, 255 });
    }
}

void UI::drawToast(const std::string& msg, float timer, bool hasBanner)
{
    if (msg.empty() || timer <= 0.0f)
    {
        return;
    }

    const int sw = GetScreenWidth();
    Vector2 sz = measureText(msg.c_str(), 14, true);
    const int w = static_cast<int>(sz.x) + 48;
    const int h = 38;
    const int x = (sw - w) / 2;
    const int y = hasBanner ? 126 : 60;

    DrawRectangleRounded(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                         0.3f, 4, Color{ 20, 28, 45, 245 });
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                              0.3f, 4, Color{ 0, 180, 240, 255 });

    drawTextCentered(msg.c_str(), x + w / 2, y + h / 2, 14, Color{ 240, 245, 255, 255 }, true);
}

void UI::drawSelfTestModal(const SelfTest& selfTest, bool& showSelfTestModal)
{
    if (!selfTest.hasRun())
    {
        return;
    }

    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();
    const int w = 520;
    const int h = 540;
    const int x = (sw - w) / 2;
    const int y = (sh - h) / 2;

    DrawRectangle(0, 0, sw, sh, Color{ 0, 0, 0, 140 });

    DrawRectangleRounded(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                         0.08f, 4, Color{ 16, 22, 34, 250 });
    DrawRectangleRoundedLines(Rectangle{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h) },
                              0.08f, 4, Color{ 60, 80, 120, 255 });

    const int passed = selfTest.getPassed();
    const int total = selfTest.getTotal();
    const bool allPassed = (total > 0 && passed == total);

    Color headerBg = allPassed ? Color{ 39, 174, 96, 255 } : Color{ 192, 57, 43, 255 };
    DrawRectangleRounded(Rectangle{ static_cast<float>(x + 12), static_cast<float>(y + 12), static_cast<float>(w - 24), 44 }, 0.2f, 4, headerBg);
    drawTextCentered(TextFormat("SELF-TEST SUITE: %d / %d PASSED", passed, total), x + w / 2, y + 34, 17, WHITE, true);

    int listY = y + 70;
    int count = 0;
    for (const std::string& line : selfTest.getResults())
    {
        if (count >= 15)
        {
            drawText("... (and more)", x + 24, listY, 13, Color{ 140, 160, 190, 255 });
            break;
        }

        const bool isOk = line.rfind("ok", 0) == 0;
        const bool isSkip = line.rfind("SKIP", 0) == 0;

        Color badgeColor = isOk ? Color{ 46, 204, 113, 255 } : (isSkip ? Color{ 241, 196, 15, 255 } : Color{ 231, 76, 60, 255 });
        DrawRectangleRounded(Rectangle{ static_cast<float>(x + 24), static_cast<float>(listY), 42, 20 }, 0.3f, 4, badgeColor);
        drawTextCentered(isOk ? "PASS" : (isSkip ? "SKIP" : "FAIL"), x + 45, listY + 10, 11, Color{ 10, 20, 30, 255 }, true);

        drawText(line.c_str(), x + 76, listY + 3, 13, Color{ 215, 225, 240, 255 });
        listY += 26;
        ++count;
    }

    DrawRectangleRounded(Rectangle{ static_cast<float>(x + w / 2 - 60), static_cast<float>(y + h - 44), 120, 30 }, 0.3f, 4, Color{ 45, 60, 90, 255 });
    drawTextCentered("Close (ESC)", x + w / 2, y + h - 29, 14, WHITE, true);

    if (IsKeyPressed(KEY_ESCAPE) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                                     GetMousePosition().y >= y + h - 44 && GetMousePosition().y <= y + h - 14 &&
                                     GetMousePosition().x >= x + w / 2 - 60 && GetMousePosition().x <= x + w / 2 + 60))
    {
        showSelfTestModal = false;
    }
}

}  // namespace urbania
