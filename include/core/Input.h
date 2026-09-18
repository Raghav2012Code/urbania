#pragma once

#include "core/Camera.h"
#include "world/Tile.h"

namespace urbania {

class Input {
public:
    void update(const Camera& camera);

    TileCoordinate hovered() const;
    bool leftClicked() const;

private:
    TileCoordinate hoveredTile{};
    bool leftClickedThisFrame = false;
};

}  // namespace urbania
