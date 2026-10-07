#include "pipeline/raster.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {
  constexpr int kSubpixelBits = 4;
  constexpr int32_t kSubpixel = 1 << kSubpixelBits; //16 steps per pixel
  constexpr int32_t kHalfPixel = kSubpixel / 2;

  struct FixedPoint {
    int32_t x, y;
  };

  FixedPoint snap(Vec2 p) {
    return {int32_t(std::lround(p.x * kSubpixel)), int32_t(std::lround(p.y * kSubpixel))};
  }

  // > 0 on one side of the line a -> b, < 0 on the other, 0 exactly on it.
  int64_t edge(FixedPoint a, FixedPoint b, int32_t px, int32_t py) {
    return int64_t(b.x-a.x) * (py-a.y) - int64_t(b.y-a.y) * (px-a.x);
  }

  // Top-left rule. With positive area in y-down screen space, a top edge
  // points right (dy == 0, dx > 0) and a left edge points up (dy < 0).
  int64_t fill_bias(FixedPoint a, FixedPoint b) {
    int32_t dx = b.x - a.x, dy = b.y - a.y;
    bool top_left = (dy == 0 && dx > 0) || dy < 0;
    return top_left ? 0 : -1;
  }

  // Calls fn(x, y, w0, w1, w2) for every pixel whose center is inside the
  // triangle and inside `clip`. Requires positive area. w_i is the edge
  // function opposite vertex i; w_i / area2 is that vertex's barycentric weight.
  template <class Fn>
  void rasterize(FixedPoint v0, FixedPoint v1, FixedPoint v2, Rect clip, Fn&& fn) {
    int min_x = (std::min({v0.x, v1.x, v2.x}) - kHalfPixel + kSubpixel - 1) >> kSubpixelBits;
    int min_y = (std::min({v0.y, v1.y, v2.y}) - kHalfPixel + kSubpixel - 1) >> kSubpixelBits;
    int max_x = (std::max({v0.x, v1.x, v2.x}) - kHalfPixel) >> kSubpixelBits;
    int max_y = (std::max({v0.y, v1.y, v2.y}) - kHalfPixel) >> kSubpixelBits;
    min_x = std::max(min_x, clip.x0);
    min_y = std::max(min_y, clip.y0);
    max_x = std::min(max_x, clip.x1 - 1);
    max_y = std::min(max_y, clip.y1 - 1);

    const int64_t bias0 = fill_bias(v1, v2), bias1 = fill_bias(v2, v0), bias2 = fill_bias(v0, v1);

    for (int y = min_y; y <= max_y; ++y) {
      for (int x = min_x; x <= max_x; ++x) {
        int32_t px = x * kSubpixel + kHalfPixel, py = y * kSubpixel + kHalfPixel;
        int64_t w0 = edge(v1, v2, px, py), w1 = edge(v2, v0, px, py), w2 = edge(v0, v1, px, py);
        if (w0 + bias0 >= 0 && w1 + bias1 >= 0 && w2 + bias2 >= 0) fn(x, y, w0, w1, w2);
      }
    }
  }

  // Snaps the three corners and returns twice the signed area.
  // 0 means degenerate; the sign is the winding on screen.
  int64_t setup(Vec2 a, Vec2 b, Vec2 c, FixedPoint out[3]) {
    out[0] = snap(a);
    out[1] = snap(b);
    out[2] = snap(c);
    return edge(out[0], out[1], out[2].x, out[2].y);
  }

} // namespace

void draw_triangle(Framebuffer& fb, const ScreenVertex& a, const ScreenVertex& b, const ScreenVertex& c, const DrawState& state) {
  const ScreenVertex* v[3] = {&a, &b, &c};
  FixedPoint p[3];
  int64_t area2 = setup(a.pos, b.pos, c.pos, p);
  if (area2 == 0) return;

  // Front faces are counter-clockwise in NDC (y up). The viewport flips y,
  // so on screen they have negative area: positive means we see the back.
  if (area2 > 0 && state.cull_backfaces) return;
  if (area2 < 0) {
    std::swap(p[1], p[2]);
    std::swap(v[1], v[2]);  // attributes must follow their positions
    area2 = -area2;
  }

  Rect clip{std::max(0, state.scissor.x0), std::max(0, state.scissor.y0),
            std::min(fb.width, state.scissor.x1), std::min(fb.height, state.scissor.y1)};
  const float inv_area = 1.0f / float(area2);

  rasterize(p[0], p[1], p[2], clip, [&](int x, int y, int64_t w0, int64_t w1, int64_t w2) {
    float l0 = float(w0) * inv_area, l1 = float(w1) * inv_area, l2 = float(w2) * inv_area;
    size_t i = size_t(y) * fb.width + x;

    // Early-z: test depth before any shading work.
    float z = l0 * v[0]->z + l1 * v[1]->z + l2 * v[2]->z;
    if (!(z < fb.depth[i])) return;
    fb.depth[i] = z;
    
    // Screen-space weights are right for depth but not for uv/normals:
    // weight each vertex by 1/w to undo the perspective squeeze.
    if (state.perspective_correct) {
      float q0 = l0 * v[0]->inv_w, q1 = l1 * v[1]->inv_w, q2 = l2 * v[2]->inv_w;
      float k = 1.0f / (q0 + q1 + q2);
      l0 = q0 * k;
      l1 = q1 * k;
      l2 = q2 * k;
    }
    Vec2 uv = v[0]->uv * l0 + v[1]->uv * l1 + v[2]->uv * l2;
    Vec3 n = v[0]->normal * l0 + v[1]->normal * l1 + v[2]->normal * l2;
    fb.color[i] = shade(state, uv, n);
  });
}

void accumulate_coverage(Vec2 a, Vec2 b, Vec2 c, int width, int height, std::vector<int>& counts) {
  FixedPoint p[3];
  int64_t area2 = setup(a, b, c, p);
  if (area2 == 0) return;
  if (area2 < 0) std::swap(p[1], p[2]);
  rasterize(p[0], p[1], p[2], Rect{0, 0, width, height},
            [&](int x, int y, int64_t, int64_t, int64_t) { ++counts[size_t(y) * width + x]; });
}
