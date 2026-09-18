# Urbania

Urbania is a 2D top-down city simulation game, built in C++ with raylib.
The long-term vision is a living city simulation covering citizens,
buildings, roads, traffic, economy, pollution, and public transportation.

> **Project status:** playable simulation foundation. You can build a
> road network, zone residential/commercial/industrial districts, watch
> citizens move in and find jobs, and fast-forward the simulation clock.
> Systems like traffic, pollution, happiness, and taxes do not exist yet.

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

## Running Urbania

```sh
./build/Urbania
```

On Windows (MSYS2 UCRT64):

```sh
./build/Urbania.exe
```

A 1920×1080 window titled `Urbania` opens at 60 FPS. Close the window
(or press `Esc`) to exit cleanly.

## Controls

| Input         | Action                                              |
| ------------- | --------------------------------------------------- |
| `W` `A` `S` `D` | Pan the camera (frame-rate independent)           |
| Mouse wheel   | Zoom (0.25x–3.0x, anchored under the cursor)        |
| Mouse move    | Hover/select a tile (works while panning/zooming)   |
| Left click    | Build on / demolish the hovered tile                |
| `1`–`5`       | Select build type: Road, Residential, Commercial, Industrial, Park |
| `D`           | Toggle demolition mode                              |
| `Space`       | Pause / resume the simulation                       |
| `F1`–`F4`     | Simulation speed: 1x, 2x, 4x, 8x                    |
| `F9`          | Run the development self-test (builds test tiles)   |

## Gameplay systems

- **World** — configurable 80×80 tile grid (`WORLD_WIDTH`,
  `WORLD_HEIGHT`, `TILE_SIZE = 32`). Tile data never depends on raylib.
- **Construction** — build on Grass for a fixed cost (Road Rs. 100,
  Residential Rs. 2,000, Commercial Rs. 5,000, Industrial Rs. 10,000,
  Park Rs. 1,000) starting from Rs. 100,000. Demolition restores Grass
  with no refund. There is no income yet.
- **Simulation clock** — city time starts at Day 1, 08:00 and advances
  independently of frame rate (1 real second = 1 sim minute at 1x).
  All simulation systems consume scaled simulation time, never raw
  frame time, so pause and fast-forward apply consistently.
- **Population** — each Residential tile houses up to 10 residents,
  growing deterministically at 1 resident per sim hour. Demolishing a
  home removes its residents.
- **Citizens** — individual entities with stable IDs, homes,
  unemployment, default income (Rs. 30,000) and neutral happiness (50).
  Simulation data only: no movement or rendering yet.
- **Employment** — Commercial tiles offer 8 jobs, Industrial tiles 15.
  Unemployed citizens are matched deterministically; demolishing a
  workplace unemploys its workers, who rematch when jobs appear.
- **Road network** — cardinal-only graph over Road tiles, rebuilt on
  road construction/demolition, with deterministic A* pathfinding
  (Manhattan heuristic) used by an on-screen debug path test.

The window also shows temporary debug info (selected tile, money, date,
speed, population, jobs, road stats). This is development UI, not the
final game UI.

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
`Game::update()` → camera/input → simulation clock → simulation
(Population → Employment) → construction, and `Game::draw()` renders the
world through the camera. The `rendering` and `ui` directories are
reserved for future systems.

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

Not yet implemented:

- Public transport (buses, trains, stations)
- Final UI, art textures, audio, and save/load persistence
