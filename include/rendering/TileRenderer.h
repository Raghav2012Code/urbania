#pragma once

#include "raylib.h"
#include "rendering/TextureManager.h"
#include "world/Tile.h"

class World;

namespace urbania {

class Congestion;
class Transit;

// High-performance procedural and textured world grid tile renderer.
// Supports 16-state road auto-tiling, architectural building variants,
// deterministic terrain noise, and bus stop road markers.
class TileRenderer {
public:
    TileRenderer();

    // Renders the entire visible world grid
    void renderWorld(const World& world, const TextureManager& textures,
                     const Congestion& congestion, const Transit& transit);

    // Renders a single tile at screen coordinates
    void renderTile(const World& world, int x, int y, const Tile& tile,
                    const TextureManager& textures, float congestionLevel,
                    bool hasBusStop, int tileSize);

    // Auto-tiling road connectivity bitmask calculation
    // Bit 0 = North, Bit 1 = East, Bit 2 = South, Bit 3 = West
    static int getRoadNeighborMask(const World& world, int x, int y);

private:
    void drawRoadTile(int x, int y, int px, int py, int tileSize, int neighborMask,
                      float congestionLevel, bool hasBusStop, const TextureManager& textures);
    void drawResidentialTile(int x, int y, int px, int py, int tileSize,
                             const TextureManager& textures);
    void drawCommercialTile(int x, int y, int px, int py, int tileSize,
                            const TextureManager& textures);
    void drawIndustrialTile(int x, int y, int px, int py, int tileSize,
                            const TextureManager& textures);
    void drawParkTile(int x, int y, int px, int py, int tileSize,
                      const TextureManager& textures);
    void drawGrassTile(int x, int y, int px, int py, int tileSize,
                       const TextureManager& textures);
};

}  // namespace urbania
