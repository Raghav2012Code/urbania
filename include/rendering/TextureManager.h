#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "raylib.h"

namespace urbania {

// Manages texture asset loading, GPU caching, and procedural fallbacks.
// Textures are loaded once on startup into GPU memory and unloaded on shutdown.
// If PNG assets under assets/textures/ do not exist on disk, procedural pixel-art
// textures are generated and cached safely.
class TextureManager {
public:
    TextureManager();
    ~TextureManager();

    // Non-copyable
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    // Initialize all texture packs and procedural fallbacks
    void initialize();

    // Release all GPU textures
    void shutdown();

    // Get texture by key (returns fallback texture if not found)
    const Texture2D* getTexture(const std::string& name) const;
    bool hasTexture(const std::string& name) const;

private:
    void loadOrGenerateTextures();
    void generateProceduralFallbacks();

    // Helpers to create and register procedural textures from Image
    void registerImage(const std::string& name, Image image);

    std::unordered_map<std::string, Texture2D> textures;
    Texture2D fallbackTexture{};
    bool initialized = false;
};

}  // namespace urbania
