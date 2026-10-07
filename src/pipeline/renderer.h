#pragma once

#include <vector>

#include "pipeline/scene.h"

struct RenderOptions {
  bool perspective_correct = true;
  bool lighting = true;
  bool cull_backfaces = true;
};

struct FrameStats {
  double vertex_ms = 0;
  double raster_ms = 0;  // clip + setup + raster + shade
};

// Runs the pipeline stages in order. Holds scratch buffers so a frame
// doesn't allocate.
class Renderer {
 public:
  RenderOptions options;

  // Frame N always looks the same: animation is driven by `frame`, not time.
  FrameStats render(Framebuffer& fb, const Scene& scene, int frame);

 private:
  std::vector<ClipVertex> clip_vertices_;
};
