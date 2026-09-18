#pragma once

#include "core/Camera.h"

namespace urbania {

struct TileCoordinate {
    int x = -1;
    int y = -1;
    bool valid = false;
};

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
