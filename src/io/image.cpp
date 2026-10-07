#include "io/image.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "stb_image.h"
#include "stb_image_write.h"

std::optional<Image> load_image(const std::string& path) {
  int w = 0, h = 0, channels = 0;
  unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 4);
  if (!data) return std::nullopt;
  Image img;
  img.width = w;
  img.height = h;
  img.pixels.resize(static_cast<size_t>(w) * h);
  std::memcpy(img.pixels.data(), data, img.pixels.size() * 4);
  stbi_image_free(data);
  return img;
}

bool write_png(const std::string& path, int width, int height, const uint32_t* pixels) {
  return stbi_write_png(path.c_str(), width, height, 4, pixels, width * 4) != 0;
}

static int channel(uint32_t p, int c) { return static_cast<int>((p >> (8 * c)) & 0xFF); }

ImageDiff compare_images(const Image& a, const Image& b) {
  ImageDiff d;
  if (a.width != b.width || a.height != b.height) {
    d.size_mismatch = true;
    return d;
  }
  for (int y = 0; y < a.height; ++y) {
    for (int x = 0; x < a.width; ++x) {
      uint32_t pa = a.at(x, y), pb = b.at(x, y);
      if (pa == pb) continue;
      if (d.differing_pixels == 0) {
        d.first_x = x;
        d.first_y = y;
      }
      ++d.differing_pixels;
      for (int c = 0; c < 4; ++c)
        d.max_channel_diff = std::max(d.max_channel_diff, std::abs(channel(pa, c) - channel(pb, c)));
    }
  }
  return d;
}

bool write_diff_png(const std::string& path, const Image& a, const Image& b) {
  if (a.width != b.width || a.height != b.height) return false;
  std::vector<uint32_t> out(a.pixels.size());
  for (size_t i = 0; i < out.size(); ++i) {
    if (a.pixels[i] != b.pixels[i]) {
      out[i] = 0xFF0000FFu;  // opaque red
    } else {
      int r = channel(a.pixels[i], 0), g = channel(a.pixels[i], 1), bl = channel(a.pixels[i], 2);
      uint32_t grey = static_cast<uint32_t>((r + g + bl) / 3 / 3);
      out[i] = grey | (grey << 8) | (grey << 16) | 0xFF000000u;
    }
  }
  return write_png(path, a.width, a.height, out.data());
}

Image make_checkerboard(int size, int cells, uint32_t c0, uint32_t c1) {
  Image img;
  img.width = img.height = size;
  img.pixels.resize(static_cast<size_t>(size) * size);
  int cell = std::max(1, size / cells);
  for (int y = 0; y < size; ++y)
    for (int x = 0; x < size; ++x)
      img.pixels[static_cast<size_t>(y) * size + x] = ((x / cell + y / cell) % 2) ? c1 : c0;
  return img;
}
