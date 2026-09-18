# Urbania — Progress Log

Chronological record of what has been built, in the order it landed.
Each entry corresponds to one commit on `main`. The working rule for
this project: finish a phase → build clean → verify live → commit →
push → confirm the remote matches. No phase starts with a dirty tree.

## Phase 1 — Project foundation (`5dd4e74`)

- C++17 + raylib + CMake + Git, no other dependencies.
- Layout: `include/` + `src/` split into `core`, `world`,
  `simulation`, `rendering`, `ui`; `assets/` with
  `textures/fonts/audio`; empty dirs kept with `.gitkeep`.
- Minimal `main.cpp` opening a window; `build/` and binaries ignored.
- Short README (stack, configure/build/run, foundation notice).

## Phase 2 — Game loop (`7bc73dc`)

- New `Game` class (`initialize/update/draw/shutdown`) owning the
  lifecycle; `main.cpp` reduced to create → init → loop → shutdown.
- 1920×1080 window titled `Urbania` at 60 FPS.
- No gameplay systems; structure reserved for world, simulation,
  rendering, UI, and core management.

## Phase 3 — README polish (`e703da4`, docs)

- Professional README: stack table, prerequisites, structure diagram,
  configure/build/run instructions, foundation-stage notice.

## Phase 4 — World grid (`4b39bcd`)

- `Tile` (`TileType`, currently all `Grass`) and `World`: configurable
  `WORLD_WIDTH = 80`, `WORLD_HEIGHT = 80`, `TILE_SIZE = 32`
  (6,400 tiles), bounds-checked accessors, no scattered literals.
- `World`/`Tile` are raylib-independent; `Game` renders the grid with
  raylib primitives plus subtle grid lines from world origin (0, 0).

## Phase 5 — Camera (`1630b7e`)

- `urbania::Camera` wrapping raylib `Camera2D` (namespaced because
  `raylib.h` already typedefs `Camera`).
- Starts centered on the world middle; WASD panning (frame-rate
  independent, zoom-scaled); mouse-wheel zoom 0.25–3.0 anchored under
  the cursor; zoom-aware bounds clamp so the map can't be lost.
- `screenToWorld` / `worldToScreen` conversions for future clicking.

## Phase 6 — Tile selection (`9e61238`)

- `urbania::Input` with a shared `TileCoordinate { x, y, valid }`
  (later moved to `world/Tile.h` so simulation can use it without
  raylib).
- Pipeline mouse → `screenToWorld()` → `floor(world / TILE_SIZE)` →
  validated tile; hover highlight, `Tile: (x, y)` / `Outside world`
  debug text, left-click detection with no action attached yet.

## Fix — Window placement (`177051b`)

- Real bug found live: on some systems the window spawned off-screen
  (measured at y=32767) and only showed as a taskbar icon.
- `Game::initialize()` now centers the window, restores it if parked
  minimized, and requests focus. Verified on-screen with pixel checks.

## Phase 7 — Building, demolition, money (`80eed8d`)

- Buildable types Road, Residential, Commercial, Industrial, Park with
  centralized costs (100 / 2,000 / 5,000 / 10,000 / 1,000) in a new
  `Economy` component; starting balance Rs. 100,000; no income yet.
- Keys `1`–`5` select type (shown with cost and balance), `D` toggles
  demolition mode. Building works only on Grass when affordable;
  demolition restores Grass with no refund. Per-type tile colors and
  state-aware hover previews (valid / occupied / unaffordable).

## Phase 8 — Simulation clock (`396a5ef`)

- `SimulationClock`: Day 1, 08:00 start; 1 real second = 1 sim minute
  at 1x; `Space` pauses/resumes; `F1`–`F4` select 1x/2x/4x/8x
  (build keys `1`–`5` deliberately untouched).
- Day/hour/minute accessors, day rollover, invalid scales rejected.
  Debug shows date plus `Speed` / red `PAUSED`.

## Phase 9 — Simulation pipeline (`1567fea`)

- New `Simulation` manager owned by `Game`; update flow is input →
  clock → `simulation.update(simulationDeltaTime)` → construction.
- Clock gained `getSimulationDeltaTime()` (zero while paused) so all
  future systems consume scaled simulation time, never `GetFrameTime()`.
- Dev-only `elapsedSimulationSeconds` counter with `Simulated:` readout
  proving the pipeline runs on simulation time.

## Phase 10 — Population and housing (`dba71d7`)

- `Population` tracks `ResidentialData { capacity, residents }` per
  residential tile (10 capacity each), growing deterministically at
  1 resident per sim hour with fractional accumulation; totals always
  recomputed; demolished homes lose their residents.
- Debug shows `Population` and `Housing: residents / capacity`.

## Phase 11 — Citizens (`6e0eaf9`)

- `Citizen { id, home, workplace, income, happiness, employed }` plus
  `CitizenManager` (vector storage, IDs from 1 never reused).
- Growth creates real entities homed to their tile: unemployed,
  invalid workplace, Rs. 30,000 income, 50 happiness. Demolition
  removes exactly that tile's citizens; survivors keep stable IDs.

## Phase 12 — Jobs and employment (`77fe15c`)

- `Employment`: Commercial tiles offer 8 jobs, Industrial 15, all else
  zero. First-unemployed to first-vacant deterministic matching;
  demolished workplaces unemploy their workers, who rematch when jobs
  appear; home demolition frees held jobs. Stats for total, occupied,
  employed, and unemployed, all shown on screen.

## Cleanup — Non-essential complexity (`6022c51`, refactor)

- Removed a dead color constant, replaced a hand-rolled clamp with
  `std::clamp`, named an inline demolish color, dropped unobservable
  camera-offset init values, and split the `CMakeLists.txt` source
  list one-per-line. Zero behavior change, verified by rebuild plus
  live camera/zoom/speed checks.

## Phase 13 — Road network (`137e573`)

- `RoadNetwork`: cardinal-only graph over Road tiles (ordered map,
  deterministic), rebuilt on demand — never per frame — and only when
  a tile changes to or from Road. Queries for membership, neighbors,
  node count, and pairwise connection.
- Debug shows `Road Nodes` plus `Road Neighbors` for hovered roads.

## Phase 14 — A* pathfinding (`ece68d8`)

- `Pathfinder::findPath()` (static): standard A* over graph nodes
  only, cost 1 per step, Manhattan heuristic, Up/Right/Down/Left
  expansion with FIFO tiebreaks; start==goal returns the single tile,
  anything invalid returns empty. No raylib dependency.
- Temporary debug test (first-to-last road, recomputed on world edits
  only) with `Path Test Length / No Path` readout and blue route
  outlines in world space.

## Docs refresh (`a51b7d9`, docs)

- README rewritten to describe the implemented game (controls table,
  systems with real numbers, true architecture and roadmap) instead
  of the day-one foundation text.

## Phase 15 — Commute routes (`a1e8b89`)

- `CommuteSystem` (owned by `Simulation`): per employed citizen, finds
  adjacent roads at home and workplace (U/R/D/L order) and stores the
  A* route on the citizen; clears routes for unemployed or unroutable
  citizens. Recalculates only when road/citizen/employment counts
  change. Stats `Commute Routes / No Route / Route Length` plus purple
  dots along a representative route.
- Fixed a real bug during implementation: a namespaced `World`
  forward declaration that poisoned lookups project-wide.

## Phase 16 — Citizen movement (`cb11dbb`)

- `Citizen` gains `currentTile` (init to home), `pathIndex` (leg
  number), `movementProgress` (0–1 per leg), `commutingToWork`;
  `CitizenMovement` advances them at 2 tiles per sim-second (pause
  freezes, speeds scale), stops at the workplace-adjacent road,
  resets the unemployed/routeless to home, and clamps replaced routes
  instead of teleporting.
- Rendering interpolates dot positions through the camera; debug adds
  `Moving Citizens` and a representative `Citizen ID: progress/total`.

## Phase 17 — Vehicle traffic (`8193f47`)

- `Vehicle { id, citizenId, path, pathIndex, movementProgress, speed,
  active }` plus `Traffic` (owned by `Simulation`, runs after citizen
  movement): one trip per employed citizen with a 2+ node route
  (dedup map, no per-frame floods), reuse of the stored commute route
  (no second A*), 4 tiles per sim-second with interpolation, finish →
  inactive → swept next update with safe respawn; demolished roads,
  unemployed or removed citizens deactivate safely.
- Rendering: red rects at interpolated positions plus orange outlines
  on one representative route; debug shows `Vehicles` and
  `Active Vehicles`.

## Phase 18 — Road capacity and congestion (`538ddae`)

- `Congestion`: road tile vehicle counts, capacity of 5 vehicles per tile,
  congestion ratio (vehicles / capacity), and non-linear speed multiplier
  slowdown curve (`1 / (1 + (ratio)^2)`).
- Congested road color overlay (transitioning towards red) and per-tile
  inspection on hover.

## Phase 19 — Basic economy and taxes (`08053ab`)

- `Economy` (owned by `Simulation`): wallet management starting at
  Rs. 100,000, safe spending with zero-clamp on deficit.
- Simulation-time daily ledger: citizen tax (Rs. 100/day per active citizen),
  workplace tax (Rs. 50/day per occupied commercial job, Rs. 75/day per
  occupied industrial job), building maintenance costs (Road Rs. 2,
  Residential Rs. 5, Commercial Rs. 10, Industrial Rs. 15, Park Rs. 5 per day).
- Settlement once per simulation day (86,400 sim-seconds) using scaled
  simulation time; pause halts ledger accumulation.
- Real-time economy HUD displaying Money, Daily Net Income, Tax, and
  Maintenance.

## Phase 20 — City demand system

- `Demand` (owned by `Simulation`): normalized [-100, +100] demand indicators
  for Residential, Commercial, and Industrial sectors.
- Simple deterministic formulas based on population, housing occupancy, and
  sector workforce balance; bounds clamped cleanly to [-100, +100].
- Hourly simulation-time updates (every 3,600 sim-seconds); pause halts
  accumulation and values remain deterministic.
- HUD integration with an overview demand block plus real-time demand preview
  on selected construction types.

## Phase 21 — Pollution simulation

- `Pollution` (owned by `Simulation`): tracks per-tile pollution in range
  [0.0, 100.0] stored in a coordinate-keyed map (`TileCoordinate` -> float).
- Industrial tiles generate pollution (+10.0/hr); natural decay clears clean
  zones (-1.0/hr); 4-cardinal diffusion spreads pollution outward; Parks reduce
  pollution locally in their tile and 4-cardinal neighbors (-4.0/hr).
- Hourly simulation-time step (every 3,600 sim-seconds); pause halts updates.
- F5 toggles semi-transparent smog overlay; HUD displays average, maximum, and
  hovered tile pollution values.

## Phase 22 — Citizen happiness system

- `Happiness` (owned by `Simulation`): authoritative calculation of individual
  citizen happiness clamped between [0.0, 100.0] starting from a 50.0 base.
- Multi-factor deterministic modifiers: Employment (+15 / -15), Housing (+5
  valid / -20 missing), Parks (+15 within radius 5 of home, no stacking),
  Pollution (linear penalty up to -30 at 100 pollution), and Commute conditions
  (-10 if unroutable, distance penalty for long routes).
- Updates once per simulation hour (every 3,600 sim-seconds) on scaled time.
- HUD displays city-wide average happiness and individual representative citizen
  happiness metrics.

## Phase 23 — Land value system

- `LandValue` (owned by `Simulation`): calculates per-tile desirability score in range
  [0.0, 100.0] stored in a coordinate-keyed map (`TileCoordinate` -> float), starting
  from a 50.0 neutral baseline.
- Multi-factor modifiers: Parks bonus (+15.0 within radius 5), linear Pollution
  penalty (-0.35 * pollution, up to -35.0), and nearby road Congestion penalty
  (up to -15.0 within radius 3).
- Calculations update once per simulation hour (every 3,600 sim-seconds) on scaled time;
  pause halts updates.
- F6 toggles color overlay (green for high land value, red/dark for low); HUD displays
  city-wide average land value and hovered tile land value.

## Phase 24 — Housing and residential value system

- `Housing` (owned by `Simulation`): tracks city-wide and per-tile residential metrics,
  housing capacity (10 residents per Residential tile), current residents, occupancy ratio,
  and housing pressure in range [-100, +100] (-100 = excess housing surplus, 0 = balanced,
  +100 = housing shortage).
- Residential Value is derived directly from `LandValue` clamped to [0.0, 100.0];
  nearby parks and pollution automatically reflect on residential desirability.
- Updates once per simulation hour (every 3,600 sim-seconds) on scaled time; pause halts updates.
- F7 toggles residential occupancy overlay (low/medium/high occupancy colored tiles);
  HUD displays detailed housing capacity, residents, occupancy percentage, housing pressure,
  and hovered residential tile resident counts and residential value.

## Phase 25 — Transit bus stop foundation

- `BusStop` & `Transit` (owned by `Simulation`): foundational public transit data structures
  and management system for bus stops with deterministic 1-based IDs.
- Placement rules: Road tiles only, at most one stop per road tile, costing ₹500
  (via `Economy`). Removal via Shift+Left Click is free and leaves the road intact.
- Road demolition automatically syncs and removes orphaned bus stops.
- Interactive mode: `B` key toggles Bus Stop placement mode with contextual previews
  (blue/cyan valid preview, red invalid/blocked outline).
- Visual rendering: Raylib-based bus stop marker with yellow background and navy sign glyph on road tiles.
- HUD displays total bus stops count (`Bus Stops: X`) and hovered bus stop ID (`Bus Stop ID: X`).

## How each phase is verified

Every feature lands only after: warning-free configure + build
(`-Wall -Wextra -Wpedantic`), a deterministic logic test compiled
against the real sources (no raylib needed for simulation code),
live in-game checks driven through the actual window (build, demolish,
pause, speeds, screenshots with pixel checks), a scoped-diff review,
and a commit + push with remote-HEAD confirmation. Simulation code
stays raylib-free throughout, proven by grep on every phase.



