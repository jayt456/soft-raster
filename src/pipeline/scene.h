#pragma once

#include <vector>

#include "io/image.h"
#include "io/mesh.h"
#include "math/mat4.h"
#include "pipeline/framebuffer.h"
#include "pipeline/vertex.h"

struct Camera {
  Vec3 eye{0, 0, 3};
  Vec3 target{0, 0, 0};
  Vec3 up{0, 1, 0};
  float fovy = 1.0472f;  // 60 degrees
  float near = 0.1f;
  float far = 100.0f;

  Mat4 view() const { return look_at(eye, target, up); }
  Mat4 projection(float aspect) const { return perspective(fovy, aspect, near, far); }
};

struct Scene {
  Mesh mesh;
  Image texture;                   // empty: flat white
  Mat4 model = Mat4::identity();
  float spin = 0;                  // radians per frame around +y
  Camera camera;
  Vec3 light_dir{0.4f, 0.8f, 0.6f};
  float ambient = 0.15f;
  bool double_sided = false;       // never cull (open meshes, camera inside)
  uint32_t background = pack_rgba(24, 24, 32);

  // Milestone 1 scenes: triangles given directly in pixel coordinates,
  // skipping the 3D stages. Three vertices per triangle, one color each.
  std::vector<ScreenVertex> screen_triangles;
  std::vector<uint32_t> screen_colors;

  size_t triangle_count() const {
    return screen_triangles.empty() ? mesh.triangle_count() : screen_triangles.size() / 3;
  }
};
