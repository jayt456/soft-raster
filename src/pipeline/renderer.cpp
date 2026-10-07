#include "pipeline/renderer.h"

#include <chrono>

#include "pipeline/clip.h"
#include "pipeline/raster.h"

namespace {

using Clock = std::chrono::steady_clock;

double ms_since(Clock::time_point t) {
  return std::chrono::duration<double, std::milli>(Clock::now() - t).count();
}

}  // namespace

FrameStats Renderer::render(Framebuffer& fb, const Scene& scene, int frame) {
  FrameStats stats;
  fb.clear(scene.background);

  DrawState state;
  state.texture = scene.texture.pixels.empty() ? nullptr : &scene.texture;
  state.light_dir = normalize(scene.light_dir);
  state.ambient = scene.ambient;
  state.lighting = options.lighting;
  state.perspective_correct = options.perspective_correct;
  state.cull_backfaces = options.cull_backfaces && !scene.double_sided;

  if (!scene.screen_triangles.empty()) {
    state.lighting = false;  // 2D triangles have no normals
    auto t = Clock::now();
    const auto& v = scene.screen_triangles;
    for (size_t i = 0; i + 2 < v.size(); i += 3) {
      state.flat_color = scene.screen_colors[i / 3];
      draw_triangle(fb, v[i], v[i + 1], v[i + 2], state);
    }
    stats.raster_ms = ms_since(t);
    return stats;
  }

  float aspect = float(fb.width) / float(fb.height);
  Mat4 model = rotate_y(scene.spin * float(frame)) * scene.model;
  Mat4 view_proj = scene.camera.projection(aspect) * scene.camera.view();

  auto t = Clock::now();
  transform_vertices(scene.mesh, model, view_proj, clip_vertices_);
  stats.vertex_ms = ms_since(t);
  if (clip_vertices_.size() != scene.mesh.positions.size()) return stats;

  t = Clock::now();
  const auto& idx = scene.mesh.indices;
  ClipVertex polygon[kMaxClipVertices];
  ScreenVertex screen[kMaxClipVertices];

  for (size_t i = 0; i + 2 < idx.size(); i += 3) {
    const ClipVertex tri[3] = {clip_vertices_[idx[i]], clip_vertices_[idx[i + 1]],
                               clip_vertices_[idx[i + 2]]};
    if (outside_frustum(tri[0], tri[1], tri[2])) continue;

    int n = clip_triangle(tri, polygon);
    for (int k = 0; k < n; ++k) screen[k] = to_screen(polygon[k], fb.width, fb.height);
    for (int k = 1; k + 1 < n; ++k) draw_triangle(fb, screen[0], screen[k], screen[k + 1], state);
  }
  stats.raster_ms = ms_since(t);
  return stats;
}
