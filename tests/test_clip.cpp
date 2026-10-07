#include <cmath>

#include "doctest.h"
#include "pipeline/clip.h"

using doctest::Approx;

namespace {

ClipVertex cv(float x, float y, float z, float w, Vec2 uv = {}) {
  ClipVertex v;
  v.pos = {x, y, z, w};
  v.uv = uv;
  return v;
}

}  // namespace

TEST_SUITE("m2") {
  TEST_CASE("to_screen: NDC corners map to framebuffer corners") {
    ScreenVertex tl = to_screen(cv(-2, 2, 1, 2), 200, 100);  // NDC (-1, 1)
    CHECK(tl.pos.x == Approx(0.0f));
    CHECK(tl.pos.y == Approx(0.0f));
    CHECK(tl.z == Approx(0.5f));
    CHECK(tl.inv_w == Approx(0.5f));

    ScreenVertex br = to_screen(cv(4, -4, 0, 4), 200, 100);  // NDC (1, -1)
    CHECK(br.pos.x == Approx(200.0f));
    CHECK(br.pos.y == Approx(100.0f));

    ScreenVertex c = to_screen(cv(0, 0, 0, 1, {0.25f, 0.75f}), 200, 100);
    CHECK(c.pos.x == Approx(100.0f));
    CHECK(c.pos.y == Approx(50.0f));
    CHECK(c.uv.x == Approx(0.25f));
  }

  TEST_CASE("triangle fully in front of the near plane passes through") {
    ClipVertex in[3] = {cv(0, 0, 0.5f, 1), cv(1, 0, 0.5f, 1), cv(0, 1, 0.5f, 1)};
    ClipVertex out[kMaxClipVertices];
    REQUIRE(clip_triangle(in, out) == 3);
    for (int i = 0; i < 3; ++i) CHECK(out[i].pos.x == in[i].pos.x);
  }

  TEST_CASE("triangle fully behind the near plane disappears") {
    ClipVertex in[3] = {cv(0, 0, -0.5f, 0.05f), cv(1, 0, -1, -1), cv(0, 1, -2, -2)};
    ClipVertex out[kMaxClipVertices];
    CHECK(clip_triangle(in, out) == 0);
  }

  TEST_CASE("one vertex behind: quad, all on or in front of the plane, attributes interpolated") {
    ClipVertex in[3] = {cv(0, 0, -1, 0.1f, {0, 0}), cv(2, 0, 1, 2, {1, 0}), cv(0, 2, 1, 2, {0, 1})};
    ClipVertex out[kMaxClipVertices];
    int n = clip_triangle(in, out);
    REQUIRE(n == 4);
    for (int i = 0; i < n; ++i) {
      CHECK(out[i].pos.z >= -1e-6f);
      CHECK(out[i].pos.w > 0.0f);
      CHECK(std::isfinite(out[i].uv.x));
    }
    // Edge v0 -> v1 crosses z = 0 halfway (z goes -1 -> 1), so uv.x there is 0.5.
    bool found_midpoint = false;
    for (int i = 0; i < n; ++i)
      if (std::fabs(out[i].pos.z) < 1e-6f && std::fabs(out[i].uv.x - 0.5f) < 1e-5f) found_midpoint = true;
    CHECK(found_midpoint);
  }

  TEST_CASE("two vertices behind: smaller triangle") {
    ClipVertex in[3] = {cv(0, 0, 1, 2), cv(1, 0, -1, 0.1f), cv(0, 1, -1, 0.1f)};
    ClipVertex out[kMaxClipVertices];
    int n = clip_triangle(in, out);
    REQUIRE(n == 3);
    for (int i = 0; i < n; ++i) CHECK(out[i].pos.z >= -1e-6f);
  }

  TEST_CASE("frustum rejection") {
    // All three right of the x = w plane.
    CHECK(outside_frustum(cv(3, 0, 0.5f, 1), cv(4, 1, 0.5f, 1), cv(5, -1, 0.5f, 1)));
    // Spanning the frustum: must not be rejected even though no vertex is inside.
    CHECK_FALSE(outside_frustum(cv(-3, 0, 0.5f, 1), cv(3, 0, 0.5f, 1), cv(0, 3, 0.5f, 1)));
    // Inside.
    CHECK_FALSE(outside_frustum(cv(0, 0, 0.5f, 1), cv(0.5f, 0, 0.5f, 1), cv(0, 0.5f, 0.5f, 1)));
  }
}
