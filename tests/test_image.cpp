#include <filesystem>

#include "doctest.h"
#include "io/image.h"
#include "pipeline/framebuffer.h"

TEST_SUITE_BEGIN("framework");

TEST_CASE("pack_rgba stores bytes as R, G, B, A in memory") {
  uint32_t p = pack_rgba(1, 2, 3, 4);
  const auto* bytes = reinterpret_cast<const uint8_t*>(&p);
  CHECK(bytes[0] == 1);
  CHECK(bytes[1] == 2);
  CHECK(bytes[2] == 3);
  CHECK(bytes[3] == 4);
}

TEST_CASE("framebuffer clear fills color and depth") {
  Framebuffer fb(7, 5);
  fb.clear(pack_rgba(10, 20, 30), 0.5f);
  for (uint32_t c : fb.color) CHECK(c == pack_rgba(10, 20, 30));
  for (float d : fb.depth) CHECK(d == 0.5f);
}

TEST_CASE("compare_images reports count, max channel diff and first pixel") {
  const uint32_t black = pack_rgba(0, 0, 0);
  Image a = make_checkerboard(16, 4, black, black);
  Image b = a;
  CHECK(compare_images(a, b).differing_pixels == 0);

  b.pixels[3 * 16 + 5] = pack_rgba(0, 7, 0);    // pixel (5, 3), green off by 7
  b.pixels[10 * 16 + 1] = pack_rgba(0, 0, 40);  // pixel (1, 10), blue off by 40
  ImageDiff d = compare_images(a, b);
  CHECK_FALSE(d.size_mismatch);
  CHECK(d.differing_pixels == 2);
  CHECK(d.first_x == 5);
  CHECK(d.first_y == 3);
  CHECK(d.max_channel_diff == 40);

  Image c = make_checkerboard(8, 4, 0, 0);
  CHECK(compare_images(a, c).size_mismatch);
}

TEST_CASE("PNG write then load round-trips exactly") {
  Image a = make_checkerboard(32, 4, pack_rgba(200, 10, 30), pack_rgba(5, 120, 250));
  auto path = std::filesystem::temp_directory_path() / "sr_roundtrip.png";
  REQUIRE(write_png(path.string(), a));
  auto b = load_image(path.string());
  REQUIRE(b.has_value());
  ImageDiff d = compare_images(a, *b);
  CHECK(d.differing_pixels == 0);
  std::filesystem::remove(path);
}

TEST_SUITE_END();
