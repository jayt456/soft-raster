#pragma once

#include <vector>

#include "io/mesh.h"
#include "math/mat4.h"

// Output of the vertex stage.
struct ClipVertex {
  Vec4 pos;     // clip space
  Vec2 uv;
  Vec3 normal;  // world space, not necessarily unit length
};

// After the perspective divide and viewport transform.
struct ScreenVertex {
  Vec2 pos;          // pixels: origin at top-left, y down, pixel centers at +0.5
  float z = 0;       // NDC depth in [0, 1]
  float inv_w = 1;   // 1 / w_clip, for perspective-correct interpolation
  Vec2 uv;
  Vec3 normal;
};

// Fills `out` with one ClipVertex per mesh vertex (same indexing as the mesh).
void transform_vertices(const Mesh& mesh, const Mat4& model, const Mat4& view_proj,
                        std::vector<ClipVertex>& out);

// Perspective divide + viewport transform. Requires v.pos.w > 0.
ScreenVertex to_screen(const ClipVertex& v, int width, int height);
