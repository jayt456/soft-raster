#include <cstdint>
#include <vector>

#include "doctest.h"
#include "pipeline/raster.h"

namespace {

constexpr int kSize = 64;

std::vector<int> new_counts() { return std::vector<int>(kSize * kSize, 0); }

// Deterministic jitter in whole 1/16-pixel steps, so jittered positions are
// exactly representable after snapping.
float jitter(uint32_t& state) {
  state = state * 1664525u + 1013904223u;
  return float(int((state >> 16) % 33) - 16) / 16.0f;  // [-1, 1] px
}

// 7x7 cells of 8 px starting at `origin`. Interior vertices are jittered;
// the outer boundary stays put so the covered area is known exactly.
void draw_grid(float origin, bool jittered, std::vector<int>& counts) {
  const int n = 7, cell = 8;
  Vec2 v[n + 1][n + 1];
  uint32_t rng = 12345;
  for (int j = 0; j <= n; ++j) {
    for (int i = 0; i <= n; ++i) {
      v[j][i] = {origin + float(i * cell), origin + float(j * cell)};
      bool interior = i > 0 && i < n && j > 0 && j < n;
      if (jittered && interior) v[j][i] = v[j][i] + Vec2{jitter(rng), jitter(rng)};
    }
  }
  for (int j = 0; j < n; ++j) {
    for (int i = 0; i < n; ++i) {
      Vec2 a = v[j][i], b = v[j][i + 1], c = v[j + 1][i + 1], d = v[j + 1][i];
      if ((i + j) % 2) {  // alternate diagonals and windings
        accumulate_coverage(a, b, c, kSize, kSize, counts);
        accumulate_coverage(a, d, c, kSize, kSize, counts);
      } else {
        accumulate_coverage(a, b, d, kSize, kSize, counts);
        accumulate_coverage(b, c, d, kSize, kSize, counts);
      }
    }
  }
}

// With the top-left rule, the 56x56 square owns exactly the pixels whose
// centers fall in [origin, origin + 56) on both axes.
void check_square_covered_once(const std::vector<int>& counts) {
  int holes = 0, doubles = 0, outside = 0;
  for (int y = 0; y < kSize; ++y) {
    for (int x = 0; x < kSize; ++x) {
      int c = counts[y * kSize + x];
      bool inside = x < 56 && y < 56;
      if (inside && c == 0) ++holes;
      if (c > 1) ++doubles;
      if (!inside && c != 0) ++outside;
    }
  }
  CHECK(holes == 0);
  CHECK(doubles == 0);
  CHECK(outside == 0);
}

}  // namespace

TEST_SUITE("m1") {
  TEST_CASE("fill rule: grid with edges between pixel centers") {
    auto counts = new_counts();
    draw_grid(0.0f, false, counts);
    check_square_covered_once(counts);
  }

  TEST_CASE("fill rule: grid with edges exactly through pixel centers") {
    auto counts = new_counts();
    draw_grid(0.5f, false, counts);
    check_square_covered_once(counts);
  }

  TEST_CASE("fill rule: jittered grid, arbitrary shared edges") {
    auto counts = new_counts();
    draw_grid(0.5f, true, counts);
    check_square_covered_once(counts);
  }

  TEST_CASE("fill rule: two triangulations of one polygon cover the same pixels") {
    const Vec2 poly[] = {{32.3f, 6.1f},  {50.7f, 13.4f}, {58.2f, 31.9f}, {50.1f, 50.6f},
                         {31.8f, 57.3f}, {13.5f, 49.8f}, {6.4f, 32.2f},  {13.9f, 13.7f}};
    const int n = 8;
    Vec2 center{32.25f, 31.75f};

    auto fan_from_vertex = new_counts(), fan_from_center = new_counts();
    for (int i = 1; i + 1 < n; ++i)
      accumulate_coverage(poly[0], poly[i], poly[i + 1], kSize, kSize, fan_from_vertex);
    for (int i = 0; i < n; ++i)
      accumulate_coverage(center, poly[i], poly[(i + 1) % n], kSize, kSize, fan_from_center);

    int covered = 0;
    for (int i = 0; i < kSize * kSize; ++i) {
      CHECK(fan_from_vertex[i] <= 1);
      CHECK(fan_from_center[i] <= 1);
      CHECK(fan_from_vertex[i] == fan_from_center[i]);
      covered += fan_from_vertex[i];
    }
    CHECK(covered > 1500);  // the octagon's area is about 1900 px
  }

  TEST_CASE("degenerate triangles draw nothing") {
    auto counts = new_counts();
    accumulate_coverage({5, 5}, {20, 20}, {35, 35}, kSize, kSize, counts);
    accumulate_coverage({10, 10}, {10, 10}, {40, 12}, kSize, kSize, counts);
    for (int c : counts) CHECK(c == 0);
  }

  TEST_CASE("triangles partly off screen are clamped, not wrapped") {
    auto counts = new_counts();
    accumulate_coverage({-30, -30}, {100, 10}, {10, 100}, kSize, kSize, counts);
    CHECK(counts[0] == 1);
    CHECK(counts[kSize * kSize - 1] == 0);
  }

  TEST_CASE("draw_triangle writes flat color") {
    Framebuffer fb(kSize, kSize);
    fb.clear(0);
    DrawState state;
    state.flat_color = pack_rgba(255, 0, 0);
    state.lighting = false;
    state.cull_backfaces = false;
    ScreenVertex a, b, c;
    a.pos = {4, 4};
    b.pos = {60, 4};
    c.pos = {4, 60};
    draw_triangle(fb, a, b, c, state);
    CHECK(fb.color[10 * kSize + 10] == pack_rgba(255, 0, 0));
    CHECK(fb.color[58 * kSize + 58] == 0u);
  }

  TEST_CASE("scissor: drawing tile by tile gives the same image as drawing once") {
    DrawState state;
    state.lighting = false;
    state.cull_backfaces = false;
    const Vec2 tris[][3] = {{{3.3f, 2.1f}, {61.7f, 9.4f}, {12.2f, 60.8f}},
                            {{61.7f, 9.4f}, {50.5f, 63.0f}, {12.2f, 60.8f}},
                            {{-10, 30}, {30, -5}, {40, 40}}};
    const uint32_t colors[] = {pack_rgba(200, 0, 0), pack_rgba(0, 200, 0), pack_rgba(0, 0, 200)};

    auto draw_all = [&](Framebuffer& fb) {
      for (int i = 0; i < 3; ++i) {
        ScreenVertex a, b, c;
        a.pos = tris[i][0];
        b.pos = tris[i][1];
        c.pos = tris[i][2];
        state.flat_color = colors[i];
        draw_triangle(fb, a, b, c, state);
      }
    };

    Framebuffer whole(kSize, kSize), tiled(kSize, kSize);
    whole.clear(0);
    tiled.clear(0);
    draw_all(whole);
    for (int ty = 0; ty < kSize; ty += 16) {
      for (int tx = 0; tx < kSize; tx += 16) {
        state.scissor = {tx, ty, tx + 16, ty + 16};
        draw_all(tiled);
      }
    }
    CHECK(whole.color[30 * kSize + 30] != 0u);
    CHECK(whole.color == tiled.color);
  }
}

TEST_SUITE("m2") {
  TEST_CASE("depth test: nearer triangle wins regardless of draw order") {
    DrawState state;
    state.lighting = false;
    state.cull_backfaces = false;

    ScreenVertex near_tri[3], far_tri[3];
    const Vec2 p[3] = {{2, 2}, {62, 8}, {8, 62}};
    const Vec2 q[3] = {{62, 62}, {2, 56}, {56, 2}};
    for (int i = 0; i < 3; ++i) {
      near_tri[i].pos = p[i];
      near_tri[i].z = 0.3f;
      far_tri[i].pos = q[i];
      far_tri[i].z = 0.6f;
    }
    const uint32_t red = pack_rgba(255, 0, 0), blue = pack_rgba(0, 0, 255);

    Framebuffer near_first(kSize, kSize), far_first(kSize, kSize);
    near_first.clear(0);
    far_first.clear(0);

    state.flat_color = red;
    draw_triangle(near_first, near_tri[0], near_tri[1], near_tri[2], state);
    state.flat_color = blue;
    draw_triangle(near_first, far_tri[0], far_tri[1], far_tri[2], state);

    state.flat_color = blue;
    draw_triangle(far_first, far_tri[0], far_tri[1], far_tri[2], state);
    state.flat_color = red;
    draw_triangle(far_first, near_tri[0], near_tri[1], near_tri[2], state);

    CHECK(near_first.color == far_first.color);
    CHECK(near_first.color[30 * kSize + 30] == red);  // inside both
  }

  TEST_CASE("back-face culling drops exactly one winding") {
    DrawState state;
    state.lighting = false;
    state.cull_backfaces = true;
    ScreenVertex a, b, c;
    a.pos = {4, 4};
    b.pos = {60, 4};
    c.pos = {4, 60};

    Framebuffer one(kSize, kSize), other(kSize, kSize);
    one.clear(0);
    other.clear(0);
    draw_triangle(one, a, b, c, state);
    draw_triangle(other, a, c, b, state);
    bool one_drawn = one.color[10 * kSize + 10] != 0;
    bool other_drawn = other.color[10 * kSize + 10] != 0;
    CHECK(one_drawn != other_drawn);
  }
}
