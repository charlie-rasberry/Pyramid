#pragma once

// Pick a scene at compile time. To switch, change ACTIVE_SCENE below
// (and rebuild). No runtime cost; the unused scene isn't even compiled in.

#include "PrismScene.h"
#include "PyramidScene.h"

// Available scenes:
//   PyramidScene   - rotating 3D ASCII pyramid (the original)
//   PrismScene     - Pink Floyd-style prism with shimmering rainbow

#define ACTIVE_SCENE PrismScene
//#define ACTIVE_SCENE PyramidScene


namespace App {
    using ActiveScene = ACTIVE_SCENE;
}
