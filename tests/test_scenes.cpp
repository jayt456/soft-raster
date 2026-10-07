#include <cmath>

#include "doctest.h"
#include "io/scenes.h"
#include "pipeline/renderer.h"

namespace {

Scene load(const char* name) {
  std::string error;
  auto scene = build_scene(name, "", &error);
  REQUIRE_MESSAGE(scene.has_value(), error);
  return std::move(*scene);
}

int drawn_pixels(const Framebuffer& fb) {
  int n = 0;
  for (float d : fb.depth) n += d < 1.0f;
  return n;
}

}  // namespace

TEST_SUITE("m2") {
  TEST_CASE("cube renders and only front faces survive culling") {
    Scene scene = load("cube");
    Renderer r;
    Framebuffer culled(320, 180), all(320, 180);
    r.render(culled, scene, 10);
    r.options.cull_backfaces = false;
    r.render(all, scene, 10);
    CHECK(drawn_pixels(culled) > 2000);
    // A closed mesh looks identical with or without culling.
    CHECK(culled.color == all.color);
  }

  TEST_CASE("camera inside a cube near a wall: no NaNs, no garbage depth") {
    Scene scene = load("inside");
    Renderer r;
    Framebuffer fb(320, 180);
    for (int frame = 0; frame < 3; ++frame) {
      r.render(fb, scene, frame);
      int bad = 0;
      for (float d : fb.depth) bad += !std::isfinite(d) || d < -1e-4f || d > 1.0f;
      CHECK(bad == 0);
    }
    // Every view direction from inside a closed box hits a wall.
    CHECK(drawn_pixels(fb) == 320 * 180);
  }
}

TEST_SUITE("m3") {
  TEST_CASE("perspective-correct and affine interpolation differ on a tilted plane") {
    Scene scene = load("plane");
    Renderer r;
    Framebuffer correct(320, 180), affine(320, 180);
    r.render(correct, scene, 0);
    r.options.perspective_correct = false;
    r.render(affine, scene, 0);

    int differing = 0;
    for (size_t i = 0; i < correct.color.size(); ++i) differing += correct.color[i] != affine.color[i];
    CHECK(differing > 1000);
  }
}
