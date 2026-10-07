#pragma once

#include <climits>
#include <cstdint>

#include "io/image.h"
#include "math/vec.h"
#include "pipeline/framebuffer.h"

// Everything draw_triangle needs besides the vertices.
struct DrawState {
  const Image* texture = nullptr;  // nullptr: every pixel uses flat_color
  uint32_t flat_color = 0xFFFFFFFFu;
  Vec3 light_dir{0, 0, 1};         // unit vector pointing toward the light
  float ambient = 0.15f;
  bool lighting = true;
  bool perspective_correct = true;
  bool cull_backfaces = true;
  // Pixels outside this rectangle are never touched (on top of the
  // framebuffer bounds). Later, one screen tile per thread.
  Rect scissor{0, 0, INT_MAX, INT_MAX};
};

// Nearest-neighbour lookup. UVs wrap (repeat). v = 0 is the bottom row of the
// image, the OBJ convention, while Image row 0 is the top.
uint32_t sample_nearest(const Image& tex, Vec2 uv);

// Final color of one pixel: texel (or flat_color) times Lambert lighting,
// each channel rounded to nearest. Alpha is always 255.
uint32_t shade(const DrawState& state, Vec2 uv, Vec3 normal);
