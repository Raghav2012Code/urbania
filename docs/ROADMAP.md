# Urbania — Development Roadmap & Future States

Welcome to the official roadmap and future state architecture for **Urbania**, a deterministic C++17 and raylib city simulation game.

---

## 🗺️ Vision & Design Philosophy

Urbania aims to combine the strategic depth, economic balance, and emergent complexity of classic city builders (such as *SimCity 2000/4000* and *Cities: Skylines*) with lightweight, blazingly fast, deterministic simulation engineering.

### Core Architectural Tenets:
1. **Strict Simulation / Rendering Separation**: All gameplay state, economics, pathfinding, and demographic calculations live in pure C++ domain models independent of the rendering loop.
2. **Determinism & Testability**: Fixed simulation time steps, seeded randomness, and in-engine diagnostic test suites ensure reproducible behavior.
3. **High-Performance Simplicity**: Minimal external dependencies (standard C++17 + raylib), cache-friendly data layouts, and linear complexity wherever possible.
4. **Readable & User-Friendly Presentation**: High-definition typography, clean glassmorphic UI, informative live heatmaps, and instant visual feedback.

---

## 📍 Current State: Baseline v0.1.0 (Alpha Release)

The v0.1.0 release establishes the foundational living city sandbox:
* **Grid World (80×80)**: Dynamic tile grid supporting Roads, Residential, Commercial, Industrial, and Parks with immediate construction costs and demolition.
* **Demographics & Housing**: Hourly citizen migration, individual Citizen entities with home and workplace coordinates, housing occupancy caps (10 residents per tile).
* **Workforce & Employment**: Commercial (8 jobs) and Industrial (15 jobs) slots, automated matching of unemployed citizens.
* **Mobility & Pathfinding**: A\* road network graph generation, citizen commute routes, vehicle trips, and dynamic congestion bottlenecks.
* **Public Transit**: Roadside bus stops, multi-stop route creation, bus entities looping routes, and citizen transit integration.
* **City Utilities**: Road-connected coverage model ($\le 2$ tiles), zoning demand rates, base capacity limits (100 kW/kL), deterministic unsupplied penalties ($-20$ happiness), and daily municipal maintenance (₹1,100/day).
* **Macroeconomics & Ledger**: Citizen taxes, workplace taxes, tile and utility upkeep, live projected cashflow, and 24-hour daily settlements.
* **Environmental Systems**: Industrial smog plume diffusion, park pollution filtration, land value desirability gradients, and composite citizen happiness scoring.
* **Visual & UI Systems**: High-res TrueType fonts with bilinear filtering, top HUD ribbon, time controls ($1\times$–$8\times$), RCI demand meter, bottom tool dock with hover tooltips, Tile Inspector, and 5-tab City Dashboard (`TAB`).
* **Overlays & Diagnostics**: Air Pollution (`F5`), Land Value (`F6`), Housing (`F7`), Utilities (`F8`), and in-engine Self-Test (`F9`).

---

## 🚀 Future Milestones & States

```
v0.1.0 (Current) ───► v0.2.0 (Save/Load & Facilities) ───► v0.3.0 (Civic Services)
                              │                                      │
                              ▼                                      ▼
                      v0.4.0 (Audio & Immersion) ───► v0.5.0 (Advanced Transit)
                              │                                      │
                              ▼                                      ▼
                      v0.6.0 (Skylines & Upgrades) ───► v1.0.0 (Scenarios & Full Launch)
```

---

### 📦 Milestone v0.2.0: Persistence & Facility Infrastructure

#### 1. City Save & Load System
* **Format**: Human-readable JSON or compact structured text/binary format without heavy external dependencies.
* **State Persistence**:
  * City metadata (City name, Mayor name, elapsed time, calendar day/hour).
  * 80×80 Tile grid states (types, development timers).
  * Citizen registry (IDs, home/work assignments, commute paths, happiness history).
  * Road graph and Transit networks (Bus stops, routes, active bus entities).
  * Treasury balance and historical financial ledger statistics.
* **Atomic Save Safety**: Write-to-temp-and-rename pattern to prevent corruption on unexpected exits.

#### 2. Physical Utility Production Facilities
* **Power Generation**:
  * *Coal Power Plant*: Low construction cost, high capacity (+500 kW), generates heavy localized air pollution.
  * *Solar Array / Wind Turbines*: High initial cost, moderate capacity (+150 kW), clean energy (0 pollution).
  * *Nuclear Plant*: Massive capacity (+2,500 kW), high upkeep, requires clean water supply.
* **Water & Sewage Infrastructure**:
  * *Water Pumping Station*: Supplies fresh water (+400 kL), must be placed away from industrial pollution plumes to avoid contamination.
  * *Water Tower*: Compact neighborhood water reserve (+100 kL).
  * *Sewage Treatment Plant*: Processes city wastewater (+400 kL), mitigates environmental pollution.

---

### 📦 Milestone v0.3.0: Civic Services & Emergency Management

#### 1. Police & Public Safety
* **Crime Dynamics**: High unemployment, low land value, and unpowered zones generate crime hotspots.
* **Police Stations & Patrols**: Police cruisers patrol local road networks, suppressing crime rates and boosting neighborhood property values.

#### 2. Fire Protection & Hazard Control
* **Fire Hazards**: Industrial zones, unwatered districts, and abandoned structures have high fire ignition probabilities.
* **Fire Stations & Response**: Fire engines dispatch along A\* road routes to actively extinguish burning buildings before fire spreads to adjacent tiles.

#### 3. Healthcare & Public Health
* **Health Index**: Smog, polluted water, and lack of sewage infrastructure degrade citizen health, causing early mortality or work absenteeism.
* **Clinics & Hospitals**: Ambulances respond to health emergencies; hospitals expand citizen lifespan and workforce productivity.

#### 4. Education & Technology
* **Elementary & High Schools**: Increase local education levels, raising commercial productivity and tax yields.
* **University**: Unlocks high-tech clean industrial zones that produce minimal pollution and generate higher tax revenue.

---

### 📦 Milestone v0.4.0: Audio, Polish & Environmental Immersion

#### 1. Spatial & Interactive Audio Engine
* **Soundscapes**:
  * Ambient city rumble scaling with population and traffic density.
  * Park birdsong, industrial machinery clatter, and commercial bustle.
* **Tactile SFX**:
  * Snappy UI button clicks, tab switches, and overlay toggle chimes.
  * Construction sounds: Bulldozer rumble, pavement laying, hammer strikes.
  * Vehicle honks, bus air-brakes, and emergency sirens.

#### 2. Day / Night Cycle & Dynamic Lighting
* **Lighting Progression**: 24-hour simulation cycle transitions smoothly from golden dawn, bright noon, dusk sunset, to dark midnight.
* **Night Illumination**:
  * Window lights illuminate across populated residential and commercial buildings.
  * Streetlamps turn on along powered roads.
  * Vehicle headlights cast subtle light beams on road tiles.

#### 3. Weather & Atmospheric Effects
* **Dynamic Weather**: Clear sunny days, heavy rain, fog, and thunderstorms.
* **Simulation Effects**: Rainy days temporarily boost water catchment but increase road traffic delays.
* **Particle FX**: Rising chimney smoke plumes from factories, bulldozer dust during demolition, and exhaust particles from buses.

---

### 📦 Milestone v0.5.0: Advanced Multi-Tier Transit

#### 1. Rail & Subway Networks
* **Overground & Underground Rail**: High-speed, high-capacity commuter rail linking distant city quadrants with zero traffic congestion.
* **Subway Stations**: Compact underground stations allowing mass transit through dense city centers without consuming surface road real estate.

#### 2. Road Network Specialization
* **Road Hierarchy**:
  * *Single-Lane Road*: Low speed, low maintenance.
  * *Four-Lane Avenue*: High capacity, central corridors.
  * *Highways / Expressways*: Grade-separated high-speed transport for long-distance industrial freight.
* **Intersections & Traffic Signals**: Roundabouts and traffic lights to manage high-volume junction flow.

#### 3. Transit Economics & Ticketing
* Municipal bus and metro fare pricing with demand elasticity (higher fares generate revenue but reduce ridership).

---

### 📦 Milestone v0.6.0: Zone Density & Urban Evolution

#### 1. Dynamic Zone Densities
* **Density Upgrades**:
  * *Low Density*: Single-family houses, small suburban shops, light workshops.
  * *Medium Density*: Row houses, 4-story apartments, shopping plazas, manufacturing plants.
  * *High Density*: High-rise condominiums, glass commercial skyscrapers, advanced corporate towers.
* **Upgrade Criteria**: Buildings automatically level up when land value is high, services (power/water/police/fire/parks) are accessible, and citizen happiness exceeds 75%.

#### 2. Unique Landmarks & Monuments
* **City Hall & Civic Plaza**: Boosts citywide happiness and increases tax efficiency.
* **Central Park & Stadium**: Major tourist attractions driving massive commercial demand and revenue.

---

### 🏆 Milestone v1.0.0: Full Release, Scenarios & Modding

#### 1. City Scenarios & Challenge Modes
* **Traffic Gridlock Crisis**: Inherit a congested metropolis and re-engineer the transit network to restore flow.
* **Disaster Recovery**: Rebuild a city recovering from an industrial explosion and economic debt.
* **Eco-Metropolis Challenge**: Achieve 25,000 population with 0% fossil fuels and zero smog footprint.

#### 2. Procedural Map Generator
* Customizable seed generator creating terrain with natural rivers, coastlines, mountain ranges, and dense forests.

#### 3. Modding & Asset Pipeline
* Custom building sprite packs and custom audio track loading via external config files.

---

## 📊 Summary Timeline

| Milestone | Target Scope | Focus Areas |
| :--- | :--- | :--- |
| **v0.1.0** | **Baseline MVP (Current)** | Core RCI loop, Bus Transit, Basic Utilities, Live Economy, Glass HUD, Overlays. |
| **v0.2.0** | **Persistence & Power** | Save/Load system, Coal/Solar Power Plants, Water Pumping Stations, Sewage Plants. |
| **v0.3.0** | **Civic Services** | Police, Fire Stations, Hospitals, Schools, Emergency dispatch vehicles. |
| **v0.4.0** | **Audio & Atmosphere** | Spatial sound engine, Day/Night lighting cycle, Weather, Particle FX. |
| **v0.5.0** | **Advanced Transit** | Subway/Metro networks, Multi-lane Avenues, Transit fare economics. |
| **v0.6.0** | **Skylines & Density** | High-density skyscrapers, dynamic building upgrades, civic landmarks. |
| **v1.0.0** | **Full Launch** | Pre-built Scenarios, Procedural Map Generator, Modding support. |

---

*Urbania is developed with C++17, raylib, and clean modular software engineering principles.*
