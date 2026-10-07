#pragma once

#include <cstdint>
#include <vector>

// RGBA8 color + float depth. Pixel (x, y) is at index y * width + x, with
// y = 0 at the top row (same as the PNG and SDL texture layout).
struct Framebuffer {
  int width = 0;
  int height = 0;
  std::vector<uint32_t> color;  // packed as bytes R, G, B, A in memory
  std::vector<float> depth;

  Framebuffer() = default;
  Framebuffer(int w, int h);

  void clear(uint32_t rgba, float depth_value = 1.0f);

  uint32_t* row(int y) { return color.data() + static_cast<size_t>(y) * width; }
  const uint32_t* row(int y) const { return color.data() + static_cast<size_t>(y) * width; }
};

// Half-open pixel rectangle [x0, x1) x [y0, y1).
struct Rect {
  int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
};

// Pack 8-bit channels so the bytes in memory are R, G, B, A (little-endian).
constexpr uint32_t pack_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
  return uint32_t(r) | (uint32_t(g) << 8) | (uint32_t(b) << 16) | (uint32_t(a) << 24);
}
