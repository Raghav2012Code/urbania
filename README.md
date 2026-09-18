# Urbania

Urbania is a 2D top-down city simulation game, built in C++ with raylib.
The long-term vision is a living city simulation covering citizens,
buildings, roads, traffic, economy, pollution, and public transportation.

> **Project status:** foundation stage. The repository currently provides
> the project setup plus a clean `Game` lifecycle and main loop. No
> gameplay or simulation systems exist yet.

## Technology stack

| Tool   | Purpose                          |
| ------ | -------------------------------- |
| C++17  | Application language             |
| raylib | Windowing, input, and rendering  |
| CMake  | Build configuration (>= 3.15)    |
| Git    | Version control                  |

No other dependencies are used.

## Prerequisites

- A C++17-capable compiler (GCC / Clang / MSVC)
- CMake >= 3.15
- raylib (available via your package manager, e.g. `pacman -S mingw-w64-ucrt-x86_64-raylib` on MSYS2 UCRT64)

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
│   ├── core/          # Game lifecycle (Game.h)
│   ├── world/
│   ├── simulation/
│   ├── rendering/
│   └── ui/
└── src/
    ├── main.cpp       # Thin application entry point
    ├── core/          # Game.cpp
    ├── world/
    ├── simulation/
    ├── rendering/
    └── ui/
```

`main.cpp` only owns the application lifecycle: it creates a `Game`
instance, runs the main loop, and shuts down. All per-frame behavior flows
through `Game::update()` and `Game::draw()`, so future systems (world,
simulation, rendering, UI, input) can be added without growing `main.cpp`.
The `world`, `simulation`, `rendering`, and `ui` directories are reserved
for those systems and intentionally remain empty for now.

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

A 1920×1080 window titled `Urbania` opens at 60 FPS with a temporary
background color. Close the window (or press `Esc`) to exit cleanly.

## Architecture

- `Game::initialize()` — creates the window and sets the target frame rate.
  Returns `false` if initialization fails.
- `Game::update(deltaTime)` — per-frame logic, driven by `GetFrameTime()`.
  Currently a placeholder for future simulation updates.
- `Game::draw()` — per-frame rendering between `BeginDrawing()` and
  `EndDrawing()`. Currently clears to a temporary background color.
- `Game::shutdown()` — releases window resources.

The project compiles warning-free with `-Wall -Wextra -Wpedantic`.

## Roadmap

Planned direction (not yet implemented):

1. World representation and rendering foundation
2. Camera and basic interaction
3. Core simulation systems (citizens, buildings, economy)
4. Traffic, pollution, and public transportation
5. UI, persistence, and polish

Each system will land as a separate, reviewable change on top of the
`Game` lifecycle established here.
