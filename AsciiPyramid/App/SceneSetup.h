#pragma once

// Pick a scene at *compile* time
// unused scenes cost no overhead

#include "PrismScene.h"
#include "PyramidScene.h"

// Available scenes:
//   PyramidScene   - rotating 3D ASCII pyramid (the original)
//   PrismScene     - Pink Floyd-style prism with shimmering rainbow

//#define ACTIVE_SCENE PrismScene
#define ACTIVE_SCENE PyramidScene


namespace App {
    using ActiveScene = ACTIVE_SCENE;
}
