#include "rendering/TextureManager.h"

#include <iostream>

namespace urbania {

TextureManager::TextureManager()
{
}

TextureManager::~TextureManager()
{
    shutdown();
}

void TextureManager::initialize()
{
    if (initialized)
    {
        return;
    }

    loadOrGenerateTextures();
    initialized = true;
}

void TextureManager::shutdown()
{
    if (!initialized)
    {
        return;
    }

    for (auto& pair : textures)
    {
        if (pair.second.id > 0)
        {
            UnloadTexture(pair.second);
        }
    }
    textures.clear();

    if (fallbackTexture.id > 0)
    {
        UnloadTexture(fallbackTexture);
        fallbackTexture = {};
    }

    initialized = false;
}

const Texture2D* TextureManager::getTexture(const std::string& name) const
{
    auto it = textures.find(name);
    if (it != textures.end())
    {
        return &(it->second);
    }
    return (fallbackTexture.id > 0) ? &fallbackTexture : nullptr;
}

bool TextureManager::hasTexture(const std::string& name) const
{
    return textures.find(name) != textures.end();
}

void TextureManager::registerImage(const std::string& name, Image image)
{
    Texture2D tex = LoadTextureFromImage(image);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);  // Crisp pixel-art scaling
    UnloadImage(image);

    // If replace
    auto it = textures.find(name);
    if (it != textures.end())
    {
        if (it->second.id > 0)
        {
            UnloadTexture(it->second);
        }
        it->second = tex;
    }
    else
    {
        textures[name] = tex;
    }
}

void TextureManager::loadOrGenerateTextures()
{
    // 1. Fallback magenta checker texture
    Image fallbackImg = GenImageChecked(32, 32, 8, 8, MAGENTA, BLACK);
    fallbackTexture = LoadTextureFromImage(fallbackImg);
    SetTextureFilter(fallbackTexture, TEXTURE_FILTER_POINT);
    UnloadImage(fallbackImg);

    // 2. Generate procedural textures (or load if file exists in assets/textures/)
    generateProceduralFallbacks();
}

void TextureManager::generateProceduralFallbacks()
{
    const int W = 32;
    const int H = 32;

    // --- TERRAIN / GRASS ---
    // grass_0 (classic lush lawn)
    {
        Image img = GenImageColor(W, H, Color{ 48, 140, 68, 255 });
        for (int y = 0; y < H; y += 4)
        {
            for (int x = 0; x < W; x += 4)
            {
                if ((x * 7 + y * 13) % 5 == 0)
                {
                    ImageDrawPixel(&img, x + 1, y + 1, Color{ 56, 155, 78, 255 });
                    ImageDrawPixel(&img, x + 2, y + 1, Color{ 40, 122, 58, 255 });
                }
            }
        }
        registerImage("terrain/grass_0", img);
    }
    // grass_1 (wildflower specks)
    {
        Image img = GenImageColor(W, H, Color{ 48, 140, 68, 255 });
        for (int y = 0; y < H; y += 6)
        {
            for (int x = 0; x < W; x += 6)
            {
                ImageDrawPixel(&img, x + 2, y + 2, Color{ 245, 230, 80, 255 }); // yellow dot
                ImageDrawPixel(&img, x + 3, y + 2, Color{ 250, 250, 250, 255 }); // white petal
            }
        }
        registerImage("terrain/grass_1", img);
    }
    // grass_2 (lush foliage tufts)
    {
        Image img = GenImageColor(W, H, Color{ 45, 135, 64, 255 });
        for (int y = 2; y < H - 2; y += 8)
        {
            for (int x = 2; x < W - 2; x += 8)
            {
                ImageDrawRectangle(&img, x, y, 3, 2, Color{ 62, 168, 86, 255 });
                ImageDrawPixel(&img, x + 1, y - 1, Color{ 68, 180, 92, 255 });
            }
        }
        registerImage("terrain/grass_2", img);
    }
    // grass_3 (earthy meadow)
    {
        Image img = GenImageColor(W, H, Color{ 50, 138, 70, 255 });
        for (int y = 0; y < H; y += 5)
        {
            for (int x = 0; x < W; x += 5)
            {
                if ((x + y) % 3 == 0)
                {
                    ImageDrawPixel(&img, x, y, Color{ 110, 120, 65, 255 });
                }
            }
        }
        registerImage("terrain/grass_3", img);
    }

    // --- ROADS ---
    // Generate base asphalt template
    auto makeRoadBase = [&]() -> Image {
        Image img = GenImageColor(W, H, Color{ 44, 48, 56, 255 });
        // subtle asphalt texture flecks
        for (int y = 0; y < H; y += 3)
        {
            for (int x = 0; x < W; x += 3)
            {
                if ((x * 11 + y * 17) % 7 == 0)
                {
                    ImageDrawPixel(&img, x, y, Color{ 52, 56, 66, 255 });
                }
            }
        }
        return img;
    };

    // Straight horizontal
    {
        Image img = makeRoadBase();
        // Sidewalk curbs top & bottom
        ImageDrawRectangle(&img, 0, 0, W, 3, Color{ 130, 135, 145, 255 });
        ImageDrawRectangle(&img, 0, H - 3, W, 3, Color{ 130, 135, 145, 255 });
        // Dashed yellow center line
        for (int x = 2; x < W; x += 8)
        {
            ImageDrawRectangle(&img, x, H / 2 - 1, 4, 2, Color{ 240, 195, 45, 255 });
        }
        registerImage("roads/straight_h", img);
    }
    // Straight vertical
    {
        Image img = makeRoadBase();
        // Sidewalk curbs left & right
        ImageDrawRectangle(&img, 0, 0, 3, H, Color{ 130, 135, 145, 255 });
        ImageDrawRectangle(&img, W - 3, 0, 3, H, Color{ 130, 135, 145, 255 });
        // Dashed yellow center line
        for (int y = 2; y < H; y += 8)
        {
            ImageDrawRectangle(&img, W / 2 - 1, y, 2, 4, Color{ 240, 195, 45, 255 });
        }
        registerImage("roads/straight_v", img);
    }
    // Crossroad 4-way
    {
        Image img = makeRoadBase();
        // Curbs on 4 corner quadrants
        ImageDrawRectangle(&img, 0, 0, 3, 3, Color{ 130, 135, 145, 255 });
        ImageDrawRectangle(&img, W - 3, 0, 3, 3, Color{ 130, 135, 145, 255 });
        ImageDrawRectangle(&img, 0, H - 3, 3, 3, Color{ 130, 135, 145, 255 });
        ImageDrawRectangle(&img, W - 3, H - 3, 3, 3, Color{ 130, 135, 145, 255 });
        // Center cross markings
        ImageDrawRectangle(&img, W / 2 - 1, 2, 2, 6, Color{ 240, 195, 45, 255 });
        ImageDrawRectangle(&img, W / 2 - 1, H - 8, 2, 6, Color{ 240, 195, 45, 255 });
        ImageDrawRectangle(&img, 2, H / 2 - 1, 6, 2, Color{ 240, 195, 45, 255 });
        ImageDrawRectangle(&img, W - 8, H / 2 - 1, 6, 2, Color{ 240, 195, 45, 255 });
        registerImage("roads/crossroad", img);
    }
    // Isolated road
    {
        Image img = makeRoadBase();
        ImageDrawRectangleLines(&img, Rectangle{ 0, 0, static_cast<float>(W), static_cast<float>(H) }, 3, Color{ 130, 135, 145, 255 });
        registerImage("roads/isolated", img);
    }

    // --- RESIDENTIAL VARIANTS ---
    // res_0: Suburban Cottage (Tile pitched roof, chimney, front door)
    {
        Image img = GenImageColor(W, H, Color{ 48, 140, 68, 255 }); // green grass lot
        // Drop shadow
        ImageDrawRectangle(&img, 6, 6, 22, 22, Color{ 20, 60, 30, 140 });
        // House base / walls
        ImageDrawRectangle(&img, 4, 4, 22, 22, Color{ 220, 210, 195, 255 });
        // Pitched roof (terracotta orange)
        ImageDrawRectangle(&img, 3, 3, 24, 14, Color{ 190, 75, 50, 255 });
        ImageDrawRectangle(&img, 5, 5, 20, 3, Color{ 215, 95, 65, 255 });
        // Brick Chimney
        ImageDrawRectangle(&img, 20, 1, 4, 6, Color{ 140, 45, 35, 255 });
        // Door & Windows
        ImageDrawRectangle(&img, 13, 19, 5, 7, Color{ 110, 60, 35, 255 });
        ImageDrawRectangle(&img, 6, 18, 5, 5, Color{ 140, 200, 240, 255 });
        ImageDrawRectangle(&img, 20, 18, 5, 5, Color{ 140, 200, 240, 255 });
        registerImage("buildings/res_0", img);
    }
    // res_1: Brick Townhouse (Slate roof, dual entrance, windows)
    {
        Image img = GenImageColor(W, H, Color{ 48, 140, 68, 255 });
        ImageDrawRectangle(&img, 5, 5, 23, 23, Color{ 20, 60, 30, 140 });
        // Brick walls
        ImageDrawRectangle(&img, 4, 4, 24, 24, Color{ 165, 80, 65, 255 });
        // Mansard dark roof
        ImageDrawRectangle(&img, 3, 3, 26, 10, Color{ 55, 65, 80, 255 });
        // 4 lit windows
        ImageDrawRectangle(&img, 6, 15, 4, 5, Color{ 255, 225, 120, 255 });
        ImageDrawRectangle(&img, 12, 15, 4, 5, Color{ 255, 225, 120, 255 });
        ImageDrawRectangle(&img, 18, 15, 4, 5, Color{ 255, 225, 120, 255 });
        ImageDrawRectangle(&img, 24, 15, 4, 5, Color{ 255, 225, 120, 255 });
        // Doors
        ImageDrawRectangle(&img, 8, 22, 5, 6, Color{ 40, 35, 30, 255 });
        ImageDrawRectangle(&img, 19, 22, 5, 6, Color{ 40, 35, 30, 255 });
        registerImage("buildings/res_1", img);
    }
    // res_2: Apartment Block (Flat roof, HVAC unit, grid windows)
    {
        Image img = GenImageColor(W, H, Color{ 48, 140, 68, 255 });
        ImageDrawRectangle(&img, 4, 4, 25, 25, Color{ 20, 60, 30, 140 });
        // Stucco building body
        ImageDrawRectangle(&img, 3, 3, 26, 26, Color{ 195, 180, 160, 255 });
        // Roof parapet & tar roof
        ImageDrawRectangle(&img, 5, 5, 22, 22, Color{ 70, 75, 85, 255 });
        // Roof vent / AC unit
        ImageDrawRectangle(&img, 8, 8, 6, 6, Color{ 140, 145, 155, 255 });
        ImageDrawRectangle(&img, 18, 16, 5, 8, Color{ 110, 115, 125, 255 });
        // Balcony / window rows
        ImageDrawRectangle(&img, 3, 27, 26, 2, Color{ 150, 135, 115, 255 });
        registerImage("buildings/res_2", img);
    }
    // res_3: Modern Multi-family Complex (Modern geometry, blue glass, gardens)
    {
        Image img = GenImageColor(W, H, Color{ 48, 140, 68, 255 });
        ImageDrawRectangle(&img, 5, 5, 24, 24, Color{ 20, 60, 30, 140 });
        ImageDrawRectangle(&img, 3, 3, 26, 26, Color{ 230, 235, 240, 255 });
        // Dark geometric accent roof
        ImageDrawRectangle(&img, 3, 3, 14, 26, Color{ 45, 55, 70, 255 });
        // Large glass windows
        ImageDrawRectangle(&img, 6, 6, 8, 18, Color{ 100, 180, 230, 255 });
        ImageDrawRectangle(&img, 19, 6, 8, 8, Color{ 100, 180, 230, 255 });
        ImageDrawRectangle(&img, 19, 16, 8, 10, Color{ 120, 90, 60, 255 }); // Wood slats
        registerImage("buildings/res_3", img);
    }

    // --- COMMERCIAL VARIANTS ---
    // com_0: Retail Bakery / Corner Shop (Red/white striped awning, display glass)
    {
        Image img = GenImageColor(W, H, Color{ 180, 185, 195, 255 }); // Paved sidewalk lot
        ImageDrawRectangle(&img, 5, 5, 24, 24, Color{ 50, 50, 60, 140 });
        // Building body
        ImageDrawRectangle(&img, 3, 3, 26, 26, Color{ 235, 215, 175, 255 });
        // Striped awning
        for (int x = 3; x < 29; x += 4)
        {
            ImageDrawRectangle(&img, x, 18, 2, 6, Color{ 220, 50, 50, 255 });
            ImageDrawRectangle(&img, x + 2, 18, 2, 6, Color{ 250, 250, 250, 255 });
        }
        // Storefront display glass
        ImageDrawRectangle(&img, 5, 24, 22, 4, Color{ 120, 200, 240, 255 });
        // Roof signage
        ImageDrawRectangle(&img, 6, 5, 20, 5, Color{ 35, 65, 115, 255 });
        registerImage("buildings/com_0", img);
    }
    // com_1: Supermarket / Mart (Large flat roof, AC condenser, loading dock)
    {
        Image img = GenImageColor(W, H, Color{ 180, 185, 195, 255 });
        ImageDrawRectangle(&img, 4, 4, 25, 25, Color{ 50, 50, 60, 140 });
        ImageDrawRectangle(&img, 2, 2, 28, 28, Color{ 60, 120, 190, 255 });
        ImageDrawRectangle(&img, 4, 4, 24, 24, Color{ 80, 88, 100, 255 });
        // Dual HVAC on roof
        ImageDrawRectangle(&img, 6, 6, 7, 7, Color{ 160, 165, 175, 255 });
        ImageDrawRectangle(&img, 18, 6, 7, 7, Color{ 160, 165, 175, 255 });
        // Yellow entrance banner
        ImageDrawRectangle(&img, 8, 26, 16, 3, Color{ 255, 205, 40, 255 });
        registerImage("buildings/com_1", img);
    }
    // com_2: Office Tower (Reflective blue windows, corporate plaza)
    {
        Image img = GenImageColor(W, H, Color{ 180, 185, 195, 255 });
        ImageDrawRectangle(&img, 5, 5, 24, 24, Color{ 50, 50, 60, 140 });
        ImageDrawRectangle(&img, 4, 4, 24, 24, Color{ 40, 55, 80, 255 });
        // Reflective glass window grid
        for (int y = 6; y <= 22; y += 4)
        {
            for (int x = 6; x <= 24; x += 4)
            {
                ImageDrawRectangle(&img, x, y, 3, 3, Color{ 130, 210, 255, 255 });
            }
        }
        registerImage("buildings/com_2", img);
    }

    // --- INDUSTRIAL VARIANTS ---
    // ind_0: Brick Factory with Smokestack
    {
        Image img = GenImageColor(W, H, Color{ 130, 125, 115, 255 }); // Industrial gravel lot
        ImageDrawRectangle(&img, 5, 5, 24, 24, Color{ 40, 40, 40, 140 });
        // Heavy brick warehouse body
        ImageDrawRectangle(&img, 3, 4, 22, 24, Color{ 130, 65, 50, 255 });
        // Corrugated metal roof
        ImageDrawRectangle(&img, 4, 5, 20, 12, Color{ 85, 95, 105, 255 });
        // Smokestack with exhaust soot
        ImageDrawRectangle(&img, 24, 2, 5, 12, Color{ 100, 45, 35, 255 });
        ImageDrawCircle(&img, 26, 2, 2, Color{ 220, 220, 220, 200 }); // Steam puff
        // Cargo roller door
        ImageDrawRectangle(&img, 7, 20, 14, 8, Color{ 180, 185, 195, 255 });
        registerImage("buildings/ind_0", img);
    }
    // ind_1: Distribution Warehouse (Loading bays & hazard stripes)
    {
        Image img = GenImageColor(W, H, Color{ 130, 125, 115, 255 });
        ImageDrawRectangle(&img, 4, 4, 25, 25, Color{ 40, 40, 40, 140 });
        ImageDrawRectangle(&img, 3, 3, 26, 26, Color{ 110, 115, 125, 255 });
        // Corrugated roof lines
        for (int x = 5; x < 27; x += 3)
        {
            ImageDrawRectangle(&img, x, 5, 1, 16, Color{ 140, 145, 155, 255 });
        }
        // Dual loading bays
        ImageDrawRectangle(&img, 5, 22, 9, 6, Color{ 40, 45, 55, 255 });
        ImageDrawRectangle(&img, 17, 22, 9, 6, Color{ 40, 45, 55, 255 });
        registerImage("buildings/ind_1", img);
    }
    // ind_2: Chemical Plant / Silos (Silver storage tanks & pipes)
    {
        Image img = GenImageColor(W, H, Color{ 130, 125, 115, 255 });
        ImageDrawRectangle(&img, 4, 4, 25, 25, Color{ 40, 40, 40, 140 });
        // Steel framework base
        ImageDrawRectangle(&img, 2, 2, 28, 28, Color{ 75, 80, 90, 255 });
        // 2 Silver Storage Silos
        ImageDrawCircle(&img, 10, 12, 6, Color{ 190, 200, 210, 255 });
        ImageDrawCircle(&img, 22, 12, 6, Color{ 190, 200, 210, 255 });
        ImageDrawCircle(&img, 10, 12, 4, Color{ 220, 230, 240, 255 });
        ImageDrawCircle(&img, 22, 12, 4, Color{ 220, 230, 240, 255 });
        // Connecting pipes
        ImageDrawRectangle(&img, 10, 11, 12, 2, Color{ 240, 160, 40, 255 });
        registerImage("buildings/ind_2", img);
    }

    // --- PARKS ---
    // park_0: Garden with Trees & Benches
    {
        Image img = GenImageColor(W, H, Color{ 45, 145, 65, 255 });
        // Cobblestone / sand winding walking path
        ImageDrawRectangle(&img, 0, 14, W, 4, Color{ 205, 190, 160, 255 });
        ImageDrawRectangle(&img, 14, 0, 4, H, Color{ 205, 190, 160, 255 });
        // 2 Big leafy trees
        ImageDrawCircle(&img, 8, 8, 5, Color{ 25, 95, 40, 255 });
        ImageDrawCircle(&img, 7, 7, 4, Color{ 40, 135, 55, 255 });
        ImageDrawCircle(&img, 24, 24, 5, Color{ 25, 95, 40, 255 });
        ImageDrawCircle(&img, 23, 23, 4, Color{ 40, 135, 55, 255 });
        // Wooden benches
        ImageDrawRectangle(&img, 19, 10, 5, 2, Color{ 120, 70, 35, 255 });
        registerImage("buildings/park_0", img);
    }
}

}  // namespace urbania
