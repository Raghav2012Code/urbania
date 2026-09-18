#pragma once

#include <string>
#include <vector>

#include "raylib.h"
#include "world/Tile.h"

class World;
class Simulation;
class SimulationClock;

namespace urbania {

class SelfTest;

// Main presentation UI system for Urbania.
// Modular, responsive, screen-space UI with top ribbon, time controller,
// SimCity-style RCI meters, bottom build dock, tabbed city dashboard,
// context-aware tile inspector, and transit route tools.
class UI {
public:
    enum class DashboardTab {
        Overview,
        Economy,
        Population,
        Transit,
        Environment
    };

    UI();

    // Updates interactive UI state (clicks on speed controls, tool dock, tabs)
    void update(SimulationClock& clock, TileType& selectedBuildType,
                bool& demolishMode, bool& busStopMode, bool& routeMode,
                bool& pollutionOverlay, bool& landValueOverlay, bool& housingOverlay,
                bool& utilitiesOverlay,
                bool& showDashboard, bool& showSelfTestModal);

    // Renders the entire Screen-Space HUD
    void draw(const World& world, const Simulation& sim,
              const SimulationClock& clock, TileType selectedBuildType,
              bool demolishMode, bool busStopMode, bool routeMode,
              bool pollutionOverlay, bool landValueOverlay, bool housingOverlay,
              bool utilitiesOverlay,
              bool showDashboard, bool showSelfTestModal,
              const TileCoordinate& hoveredTile,
              const std::vector<int>& currentRouteStops,
              const std::string& toastMessage, float toastTimer,
              const SelfTest& selfTest);

    // Checks if the mouse cursor is currently over any interactive HUD window
    bool isMouseOverUI() const;

private:
    void drawTopRibbon(const Simulation& sim, const SimulationClock& clock,
                       bool pollutionOverlay, bool landValueOverlay, bool housingOverlay,
                       bool utilitiesOverlay,
                       bool showDashboard, bool showSelfTestModal);
    void drawDemandMeters(const Simulation& sim);
    void drawBottomDock(const Simulation& sim, TileType selectedBuildType,
                        bool demolishMode, bool busStopMode, bool routeMode);
    void drawDashboard(const World& world, const Simulation& sim);
    void drawTileInspector(const World& world, const Simulation& sim,
                           const TileCoordinate& hovered);
    void drawOverlayLegends(const Simulation& sim, bool pollutionOverlay, bool landValueOverlay,
                            bool housingOverlay, bool utilitiesOverlay);
    void drawModeBanners(bool demolishMode, bool busStopMode, bool routeMode,
                         const std::vector<int>& currentRouteStops);
    void drawToast(const std::string& msg, float timer, bool hasBanner);
    void drawSelfTestModal(const SelfTest& selfTest, bool& showSelfTestModal);

    DashboardTab currentTab = DashboardTab::Overview;
    bool mouseOverUI = false;
};

}  // namespace urbania
