# Urbania

Urbania is a 2D top-down city simulation game. The long-term goal is to
simulate citizens, buildings, roads, traffic, economy, pollution, and
public transportation.

> The project is currently in its **foundation stage**: this repository
> contains only the initial C++ project setup. No gameplay systems exist yet.

## Technology stack

* C++17
* [raylib](https://www.raylib.com/) for windowing and rendering
* CMake for configuration and builds
* Git for version control

No other dependencies are used.

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
│   ├── core/
│   ├── world/
│   ├── simulation/
│   ├── rendering/
│   └── ui/
└── src/
    ├── main.cpp
    ├── core/
    ├── world/
    ├── simulation/
    ├── rendering/
    └── ui/
```

`main.cpp` is a minimal raylib entry point. The `core`, `world`,
`simulation`, `rendering`, and `ui` directories are reserved for future
systems and intentionally remain empty for now.

## Configure and build

Requirements: a C++17 compiler, CMake (>= 3.15), and raylib.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

## Run

```sh
./build/Urbania
```

On Windows (MSYS2 UCRT64):

```sh
./build/Urbania.exe
```

You should see an 1920x1080 window titled "Urbania". Close the window to exit.
