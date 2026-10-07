#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// RGBA8 image, same pixel packing as Framebuffer::color.
struct Image {
  int width = 0;
  int height = 0;
  std::vector<uint32_t> pixels;

  uint32_t at(int x, int y) const { return pixels[static_cast<size_t>(y) * width + x]; }
};

std::optional<Image> load_image(const std::string& path);
bool write_png(const std::string& path, int width, int height, const uint32_t* pixels);
inline bool write_png(const std::string& path, const Image& img) {
  return write_png(path, img.width, img.height, img.pixels.data());
}

// Per-pixel comparison used by golden-image tests and the SIMD/threaded
// equivalence checks.
struct ImageDiff {
  bool size_mismatch = false;
  int differing_pixels = 0;
  int max_channel_diff = 0;  // largest |a - b| over any single R/G/B/A channel
  int first_x = -1, first_y = -1;  // first differing pixel, for debugging
};

ImageDiff compare_images(const Image& a, const Image& b);

// Writes a visualisation of where two images differ: differing pixels in red,
// matching pixels as dimmed grey copies of `a`.
bool write_diff_png(const std::string& path, const Image& a, const Image& b);

// Procedural checkerboard texture, `cells` x `cells` squares.
Image make_checkerboard(int size, int cells, uint32_t c0, uint32_t c1);
