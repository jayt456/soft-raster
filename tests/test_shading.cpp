#include "doctest.h"
#include "pipeline/shading.h"

namespace {

// 4x4 texture where every texel is distinct: red = column, green = row.
Image numbered_texture() {
  Image img;
  img.width = img.height = 4;
  for (int y = 0; y < 4; ++y)
    for (int x = 0; x < 4; ++x) img.pixels.push_back(pack_rgba(uint8_t(x), uint8_t(y), 0));
  return img;
}

}  // namespace

TEST_SUITE("m3") {
  TEST_CASE("sample_nearest: v = 0 is the bottom row") {
    Image tex = numbered_texture();
    CHECK(sample_nearest(tex, {0.1f, 0.1f}) == pack_rgba(0, 3, 0));   // bottom-left
    CHECK(sample_nearest(tex, {0.9f, 0.9f}) == pack_rgba(3, 0, 0));   // top-right
    CHECK(sample_nearest(tex, {0.6f, 0.4f}) == pack_rgba(2, 2, 0));
  }

  TEST_CASE("sample_nearest: uvs wrap and stay in bounds") {
    Image tex = numbered_texture();
    CHECK(sample_nearest(tex, {1.1f, 0.1f}) == sample_nearest(tex, {0.1f, 0.1f}));
    CHECK(sample_nearest(tex, {-0.9f, 0.1f}) == sample_nearest(tex, {0.1f, 0.1f}));
    CHECK(sample_nearest(tex, {0.1f, 3.1f}) == sample_nearest(tex, {0.1f, 0.1f}));
    sample_nearest(tex, {1.0f, 1.0f});  // exact edge: must not read out of bounds (ASan)
    sample_nearest(tex, {0.0f, 0.0f});
  }

  TEST_CASE("shade: lighting off returns the texel unchanged") {
    Image tex = make_checkerboard(8, 2, pack_rgba(200, 100, 50), pack_rgba(10, 20, 30));
    DrawState s;
    s.texture = &tex;
    s.lighting = false;
    CHECK(shade(s, {0.1f, 0.9f}, {0, 0, 0}) == sample_nearest(tex, {0.1f, 0.9f}));
  }

  TEST_CASE("shade: Lambert") {
    DrawState s;
    s.flat_color = pack_rgba(200, 104, 40);
    s.light_dir = {0, 1, 0};
    s.ambient = 0.25f;

    // Facing the light: full brightness. Normal length must not matter.
    CHECK(shade(s, {}, {0, 5, 0}) == pack_rgba(200, 104, 40));
    // Facing away: ambient only.
    CHECK(shade(s, {}, {0, -1, 0}) == pack_rgba(50, 26, 10));
    // 60 degrees off: 0.25 + 0.75 * 0.5 = 0.625.
    CHECK(shade(s, {}, {0.8660254f, 0.5f, 0}) == pack_rgba(125, 65, 25));
  }

  TEST_CASE("shade keeps alpha opaque") {
    DrawState s;
    s.flat_color = pack_rgba(200, 100, 40, 255);
    s.light_dir = {0, 1, 0};
    uint32_t c = shade(s, {}, {0, -1, 0});
    CHECK((c >> 24) == 255u);
  }
}
