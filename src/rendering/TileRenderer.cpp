#include "rendering/TileRenderer.h"

#include <algorithm>
#include <cmath>

#include "simulation/Congestion.h"
#include "simulation/Transit.h"
#include "world/World.h"

namespace urbania {

TileRenderer::TileRenderer()
{
}

int TileRenderer::getRoadNeighborMask(const World& world, int x, int y)
{
    int mask = 0;
    // North (y - 1) -> bit 0
    if (y > 0 && world.getTile(x, y - 1).type == TileType::Road)
    {
        mask |= 1;
    }
    // East (x + 1) -> bit 1
    if (x + 1 < world.getWidth() && world.getTile(x + 1, y).type == TileType::Road)
    {
        mask |= 2;
    }
    // South (y + 1) -> bit 2
    if (y + 1 < world.getHeight() && world.getTile(x, y + 1).type == TileType::Road)
    {
        mask |= 4;
    }
    // West (x - 1) -> bit 3
    if (x > 0 && world.getTile(x - 1, y).type == TileType::Road)
    {
        mask |= 8;
    }
    return mask;
}

void TileRenderer::renderWorld(const World& world, const TextureManager& textures,
                               const Congestion& congestion, const Transit& transit)
{
    const int tileSize = world.getTileSize();
    const int w = world.getWidth();
    const int h = world.getHeight();

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            const Tile& tile = world.getTile(x, y);
            const float cong = (tile.type == TileType::Road) ? congestion.getCongestion(x, y) : 0.0f;
            const bool busStop = transit.hasBusStop(x, y);
            renderTile(world, x, y, tile, textures, cong, busStop, tileSize);
        }
    }

    // Subtle grid overlay lines
    const int gridWidth = w * tileSize;
    const int gridHeight = h * tileSize;
    const Color gridCol = Color{ 0, 0, 0, 28 };

    for (int x = 0; x <= w; ++x)
    {
        DrawLine(x * tileSize, 0, x * tileSize, gridHeight, gridCol);
    }
    for (int y = 0; y <= h; ++y)
    {
        DrawLine(0, y * tileSize, gridWidth, y * tileSize, gridCol);
    }
}

void TileRenderer::renderTile(const World& world, int x, int y, const Tile& tile,
                              const TextureManager& textures, float congestionLevel,
                              bool hasBusStop, int tileSize)
{
    const int px = x * tileSize;
    const int py = y * tileSize;

    switch (tile.type)
    {
    case TileType::Grass:
        drawGrassTile(x, y, px, py, tileSize, textures);
        break;

    case TileType::Road:
    {
        const int mask = getRoadNeighborMask(world, x, y);
        drawRoadTile(x, y, px, py, tileSize, mask, congestionLevel, hasBusStop, textures);
        break;
    }

    case TileType::Residential:
        drawResidentialTile(x, y, px, py, tileSize, textures);
        break;

    case TileType::Commercial:
        drawCommercialTile(x, y, px, py, tileSize, textures);
        break;

    case TileType::Industrial:
        drawIndustrialTile(x, y, px, py, tileSize, textures);
        break;

    case TileType::Park:
        drawParkTile(x, y, px, py, tileSize, textures);
        break;

    default:
        drawGrassTile(x, y, px, py, tileSize, textures);
        break;
    }
}

void TileRenderer::drawGrassTile(int x, int y, int px, int py, int tileSize,
                                 const TextureManager& textures)
{
    // Deterministic variant (0 to 3)
    const unsigned int hash = static_cast<unsigned int>(x * 73856093 ^ y * 19349663);
    const int variant = hash % 4;

    const std::string texName = "terrain/grass_" + std::to_string(variant);
    const Texture2D* tex = textures.getTexture(texName);

    if (tex != nullptr && tex->id > 0)
    {
        Rectangle src = { 0.0f, 0.0f, static_cast<float>(tex->width), static_cast<float>(tex->height) };
        Rectangle dst = { static_cast<float>(px), static_cast<float>(py),
                          static_cast<float>(tileSize), static_cast<float>(tileSize) };
        DrawTexturePro(*tex, src, dst, Vector2{ 0, 0 }, 0.0f, WHITE);
    }
    else
    {
        DrawRectangle(px, py, tileSize, tileSize, Color{ 48, 140, 68, 255 });
    }
}

void TileRenderer::drawRoadTile(int x, int y, int px, int py, int tileSize, int neighborMask,
                                float congestionLevel, bool hasBusStop, const TextureManager& textures)
{
    (void)x;
    (void)y;
    (void)textures;
    const float ts = static_cast<float>(tileSize);
    const float curb = 3.0f;
    const float mid = ts / 2.0f;

    // 1. Base dark asphalt
    DrawRectangle(px, py, tileSize, tileSize, Color{ 44, 48, 56, 255 });

    const bool n = (neighborMask & 1) != 0;
    const bool e = (neighborMask & 2) != 0;
    const bool s = (neighborMask & 4) != 0;
    const bool w = (neighborMask & 8) != 0;

    const Color curbCol = Color{ 135, 140, 150, 255 };
    const Color yellowLine = Color{ 245, 200, 45, 255 };

    // 2. Draw sidewalk curbs on sides where there are no neighbor roads
    if (!n) DrawRectangle(px, py, tileSize, static_cast<int>(curb), curbCol);
    if (!s) DrawRectangle(px, py + tileSize - static_cast<int>(curb), tileSize, static_cast<int>(curb), curbCol);
    if (!w) DrawRectangle(px, py, static_cast<int>(curb), tileSize, curbCol);
    if (!e) DrawRectangle(px + tileSize - static_cast<int>(curb), py, static_cast<int>(curb), tileSize, curbCol);

    // 3. Center road lane markings based on connectivity
    if (neighborMask == 0)
    {
        // Isolated road tile
        DrawRectangle(px + static_cast<int>(mid) - 2, py + static_cast<int>(mid) - 2, 4, 4, yellowLine);
    }
    else if ((w || e) && !n && !s)
    {
        // Pure horizontal road
        DrawRectangle(px, py + static_cast<int>(mid) - 1, tileSize, 2, yellowLine);
    }
    else if ((n || s) && !w && !e)
    {
        // Pure vertical road
        DrawRectangle(px + static_cast<int>(mid) - 1, py, 2, tileSize, yellowLine);
    }
    else
    {
        // Intersection, T-junction, or corner
        DrawCircle(px + static_cast<int>(mid), py + static_cast<int>(mid), 2.5f, yellowLine);
        if (n) DrawRectangle(px + static_cast<int>(mid) - 1, py, 2, static_cast<int>(mid), yellowLine);
        if (s) DrawRectangle(px + static_cast<int>(mid) - 1, py + static_cast<int>(mid), 2, static_cast<int>(mid), yellowLine);
        if (w) DrawRectangle(px, py + static_cast<int>(mid) - 1, static_cast<int>(mid), 2, yellowLine);
        if (e) DrawRectangle(px + static_cast<int>(mid), py + static_cast<int>(mid) - 1, static_cast<int>(mid), 2, yellowLine);
    }

    // 4. Congestion heat tint overlay
    if (congestionLevel > 0.4f)
    {
        const float alphaFactor = std::clamp((congestionLevel - 0.4f) / 1.6f, 0.0f, 1.0f);
        const unsigned char alpha = static_cast<unsigned char>(alphaFactor * 140.0f);
        const Color heat = (congestionLevel > 1.2f) ? Color{ 220, 40, 40, alpha } : Color{ 240, 140, 30, alpha };
        DrawRectangle(px, py, tileSize, tileSize, heat);
    }

    // 5. Bus Stop road markings
    if (hasBusStop)
    {
        // Yellow road transit border and "BUS" sign shelter
        DrawRectangleLines(px + 2, py + 2, tileSize - 4, tileSize - 4, Color{ 255, 215, 0, 200 });
        DrawRectangle(px + tileSize - 8, py + 2, 6, 6, Color{ 255, 215, 0, 255 });
        DrawText("B", px + tileSize - 7, py + 2, 8, Color{ 20, 50, 120, 255 });
    }
}

void TileRenderer::drawResidentialTile(int x, int y, int px, int py, int tileSize,
                                       const TextureManager& textures)
{
    const int variant = (x * 37 + y * 19) % 4;
    const std::string texName = "buildings/res_" + std::to_string(variant);
    const Texture2D* tex = textures.getTexture(texName);

    if (tex != nullptr && tex->id > 0)
    {
        Rectangle src = { 0.0f, 0.0f, static_cast<float>(tex->width), static_cast<float>(tex->height) };
        Rectangle dst = { static_cast<float>(px), static_cast<float>(py),
                          static_cast<float>(tileSize), static_cast<float>(tileSize) };
        DrawTexturePro(*tex, src, dst, Vector2{ 0, 0 }, 0.0f, WHITE);
    }
    else
    {
        DrawRectangle(px, py, tileSize, tileSize, Color{ 70, 130, 220, 255 });
    }
}

void TileRenderer::drawCommercialTile(int x, int y, int px, int py, int tileSize,
                                      const TextureManager& textures)
{
    const int variant = (x * 43 + y * 29) % 3;
    const std::string texName = "buildings/com_" + std::to_string(variant);
    const Texture2D* tex = textures.getTexture(texName);

    if (tex != nullptr && tex->id > 0)
    {
        Rectangle src = { 0.0f, 0.0f, static_cast<float>(tex->width), static_cast<float>(tex->height) };
        Rectangle dst = { static_cast<float>(px), static_cast<float>(py),
                          static_cast<float>(tileSize), static_cast<float>(tileSize) };
        DrawTexturePro(*tex, src, dst, Vector2{ 0, 0 }, 0.0f, WHITE);
    }
    else
    {
        DrawRectangle(px, py, tileSize, tileSize, Color{ 240, 160, 40, 255 });
    }
}

void TileRenderer::drawIndustrialTile(int x, int y, int px, int py, int tileSize,
                                      const TextureManager& textures)
{
    const int variant = (x * 53 + y * 31) % 3;
    const std::string texName = "buildings/ind_" + std::to_string(variant);
    const Texture2D* tex = textures.getTexture(texName);

    if (tex != nullptr && tex->id > 0)
    {
        Rectangle src = { 0.0f, 0.0f, static_cast<float>(tex->width), static_cast<float>(tex->height) };
        Rectangle dst = { static_cast<float>(px), static_cast<float>(py),
                          static_cast<float>(tileSize), static_cast<float>(tileSize) };
        DrawTexturePro(*tex, src, dst, Vector2{ 0, 0 }, 0.0f, WHITE);
    }
    else
    {
        DrawRectangle(px, py, tileSize, tileSize, Color{ 175, 75, 75, 255 });
    }
}

void TileRenderer::drawParkTile(int x, int y, int px, int py, int tileSize,
                                const TextureManager& textures)
{
    (void)x;
    (void)y;
    const Texture2D* tex = textures.getTexture("buildings/park_0");

    if (tex != nullptr && tex->id > 0)
    {
        Rectangle src = { 0.0f, 0.0f, static_cast<float>(tex->width), static_cast<float>(tex->height) };
        Rectangle dst = { static_cast<float>(px), static_cast<float>(py),
                          static_cast<float>(tileSize), static_cast<float>(tileSize) };
        DrawTexturePro(*tex, src, dst, Vector2{ 0, 0 }, 0.0f, WHITE);
    }
    else
    {
        DrawRectangle(px, py, tileSize, tileSize, Color{ 35, 140, 60, 255 });
    }
}

}  // namespace urbania
