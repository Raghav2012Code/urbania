# Urbania

Urbania is a 2D top-down city simulation game, built in C++17 with raylib.

## Download & play (Windows x64)

No developer tools needed:

1. Download `Urbania-Windows-x64.zip` from
   [GitHub Releases](https://github.com/Raghav2012Code/urbania/releases).
2. Extract it anywhere.
3. Run `Urbania.exe` and build your city.

> **Project status:** playable, polished city simulation. Build roads,
> zone residential/commercial/industrial districts and parks, watch
> citizens move in, find jobs and commute by car or bus, manage
> happiness, pollution, land value, utilities and the city budget —
> with pause, 1x–8x speeds, overlays, day/night atmosphere, and
> save/load (`saves/urbania_save.dat`, created next to the game).

## Technology stack

| Tool   | Purpose                          |
| ------ | -------------------------------- |
| C++17  | Application language             |
| raylib | Windowing, input, and rendering  |
| CMake  | Build configuration (>= 3.15)    |
| Git    | Version control                  |

No other dependencies are used. The project compiles warning-free with
`-Wall -Wextra -Wpedantic`.

## Prerequisites

- A C++17-capable compiler (GCC / Clang / MSVC)
- CMake >= 3.15
- raylib (available via your package manager, e.g. `pacman -S mingw-w64-ucrt-x86_64-raylib` on MSYS2 UCRT64)

## Getting started

Configure the project:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
```

Build the executable:

```sh
cmake --build build
```

For an optimized player build:

```sh
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --parallel
```

## Running Urbania

```sh
./build/Urbania
```

On Windows (MSYS2 UCRT64):

```sh
./build/Urbania.exe
```

A 1280×720 window titled `Urbania` opens at 60 FPS (resizable). Close the
window (or press `Esc`) to exit cleanly.

## Controls

| Input         | Action                                              |
| ------------- | --------------------------------------------------- |
| `W` `A` `S` `D` | Pan the camera (frame-rate independent)           |
| Mouse wheel   | Zoom (0.25x–3.0x, anchored under the cursor)        |
| Mouse move    | Hover/select a tile (works while panning/zooming)   |
| Left click    | Build on / demolish the hovered tile                |
| `1`–`5`       | Select build type: Road, Residential, Commercial, Industrial, Park |
| `B`           | Toggle bus stop placement mode (Shift+Click to remove) |
| `R`           | Toggle bus route creation mode (Shift+R to delete latest route) |
| `X`           | Toggle demolition mode (frees `WASD` camera pan)     |
| `Space`       | Pause / resume the simulation                       |
| `F1`–`F4`     | Simulation speed: 1x, 2x, 4x, 8x                    |
| `F5`          | Save city (`saves/urbania_save.dat`)                |
| `F6`          | Load city                                           |
| `F7`          | Toggle pollution overlay                            |
| `F8`          | Toggle land value overlay                           |
| `F9`          | Toggle housing occupancy overlay                    |
| `F10`         | Toggle utilities overlay                            |
| `F11`         | Run the development self-test (builds test tiles)   |
| `TAB`         | Toggle the 5-tab city dashboard                     |

## Gameplay systems

- **World** — configurable 80×80 tile grid (`WORLD_WIDTH`,
  `WORLD_HEIGHT`, `TILE_SIZE = 32`). Tile data never depends on raylib.
- **Construction** — build on Grass for a fixed cost (Road Rs. 100,
  Residential Rs. 2,000, Commercial Rs. 5,000, Industrial Rs. 10,000,
  Park Rs. 1,000) starting from Rs. 100,000. Bus stops cost Rs. 500.
  Demolition restores Grass with no refund. Citizen and workplace taxes,
  plus tile and utility maintenance, are settled every simulated day.
- **Simulation clock** — city time starts at Day 1, 08:00 and advances
  independently of frame rate (1 real second = 1 sim minute at 1x).
  All simulation systems consume scaled simulation time, never raw
  frame time, so pause and fast-forward apply consistently.
- **Population** — each Residential tile houses up to 10 residents,
  growing deterministically at 1 resident per sim hour. Demolishing a
  home removes its residents.
- **Citizens** — individual entities with stable IDs, homes,
  unemployment, default income (Rs. 30,000) and neutral happiness (50).
  Employed citizens commute along road routes, rendered as cars, and can
  use the bus network.
- **Employment** — Commercial tiles offer 8 jobs, Industrial tiles 15.
  Unemployed citizens are matched deterministically; demolishing a
  workplace unemploys its workers, who rematch when jobs appear.
- **Road network** — cardinal-only graph over Road tiles, rebuilt on
  road construction/demolition, with deterministic A* pathfinding
  (Manhattan heuristic) used by an on-screen debug path test.

The window shows a top HUD ribbon (money, date and time, speed, population,
and RCI demand), a bottom tool dock with hover tooltips, a tile inspector,
and a 5-tab city dashboard opened with `TAB`.

## Project structure

```text
Urbania/
├── CMakeLists.txt
├── README.md
├── .gitignore
├── assets/
│   ├── textures/
│   ├── fonts/
│   └── audio/
├── include/
│   ├── core/          # Game, Camera, Input
│   ├── world/         # Tile, World
│   ├── simulation/    # Economy, SimulationClock, Simulation,
│   │                  # Population, Citizen, Employment, RoadNetwork, Pathfinder
│   ├── rendering/
│   └── ui/
└── src/
    ├── main.cpp       # Thin application entry point
    ├── core/
    ├── world/
    ├── simulation/
    ├── rendering/
    └── ui/
```

`main.cpp` only owns the application lifecycle. Per-frame behavior flows
`Game::update()` → camera/input → simulation clock → simulation → construction,
and `Game::draw()` renders the world through the camera. `rendering/` holds
the tile and entity renderers plus procedural textures, and `ui/` holds the
HUD, tool dock, tile inspector, and city dashboard.

## Roadmap

Done:

- Project setup, game loop, and window management
- Tile grid, camera (pan/zoom), and mouse tile selection
- Construction, demolition, and money wallet
- Simulation clock with pause and speeds (1x, 2x, 4x, 8x)
- Simulation pipeline, population, citizens, employment
- Road network with A* pathfinding
- Citizen commute routes and visual movement
- Vehicle traffic and road congestion slowdowns
- Basic economy, daily citizen/job taxes, and tile maintenance
- City demand indicators (Residential, Commercial, Industrial)
- Pollution simulation (Industrial emissions, 4-cardinal diffusion, park filtering, and decay)
- Citizen happiness system (Employment, Housing, Parks, Commutes, and Pollution)
- Land value simulation (Tile desirability based on parks, pollution, and congestion)
- Housing and residential value (Housing capacity, occupancy ratio, pressure, and residential value)
- Public transit foundation (Bus stops, placement rules, removal, road sync, and visual rendering)
- Bus routes (Ordered bus-stop sequences, road connectivity validation, route creation and deletion)
- Moving bus vehicles, city utilities with daily maintenance, and overlays
- Save/load (`F5`/`F6`), procedural textures, and day/night atmosphere

Not yet implemented:

- Transit passenger simulation and bus line schedules
- Power plants and water towers to raise utility capacity
- Trains, stations, and rail networks
- Audio
