#pragma once

#include "pipeline/renderer.h"

// Interactive viewer. Keys: Space pause, P perspective-correct on/off,
// C back-face culling on/off, L lighting on/off, Esc quit.
int run_window(const Scene& scene, Renderer& renderer, int width, int height);
