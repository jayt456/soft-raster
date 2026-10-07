#include "pipeline/shading.h"

#include <algorithm>
#include <cmath>

uint32_t sample_nearest(const Image& tex, Vec2 uv) {
  float u = uv.x - std::floor(uv.x);  // wrap into [0, 1)
  float v = uv.y - std::floor(uv.y);
  int x = std::min(int(u * float(tex.width)), tex.width - 1);
  int y = std::min(int((1.0f - v) * float(tex.height)), tex.height - 1);  // v = 0 is the bottom row
  return tex.at(x, y);
}

uint32_t shade(const DrawState& state, Vec2 uv, Vec3 normal) {
  uint32_t color = state.texture ? sample_nearest(*state.texture, uv) : state.flat_color;
  if (!state.lighting) return color;

  float ndotl = std::max(0.0f, dot(normalize(normal), state.light_dir));
  float k = state.ambient + (1.0f - state.ambient) * ndotl;

  auto channel = [&](int shift) {
    float c = float((color >> shift) & 0xFF) * k + 0.5f;  // + 0.5 rounds to nearest
    return uint8_t(std::min(c, 255.0f));
  };
  return pack_rgba(channel(0), channel(8), channel(16), 255);
}
