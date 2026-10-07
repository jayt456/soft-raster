#include "doctest.h"
#include "math/mat4.h"

using doctest::Approx;

namespace {

void check_vec(Vec4 a, Vec4 b) {
  CHECK(a.x == Approx(b.x).epsilon(1e-5));
  CHECK(a.y == Approx(b.y).epsilon(1e-5));
  CHECK(a.z == Approx(b.z).epsilon(1e-5));
  CHECK(a.w == Approx(b.w).epsilon(1e-5));
}

}  // namespace

TEST_SUITE("m2") {
  TEST_CASE("matrix * vector") {
    Mat4 m;
    for (int i = 0; i < 16; ++i) m.m[i] = float(i + 1);  // column-major
    // Row 0 is m[0], m[4], m[8], m[12] = 1, 5, 9, 13.
    check_vec(m * Vec4{1, 0, 0, 0}, {1, 2, 3, 4});
    check_vec(m * Vec4{0, 1, 0, 0}, {5, 6, 7, 8});
    check_vec(m * Vec4{1, 1, 1, 1}, {28, 32, 36, 40});
    check_vec(Mat4::identity() * Vec4{3, -2, 5, 1}, {3, -2, 5, 1});
  }

  TEST_CASE("matrix * matrix applies the right-hand matrix first") {
    Mat4 t = translate({10, 0, 0});
    Mat4 s = scale(2);
    check_vec((t * s) * Vec4{1, 1, 1, 1}, {12, 2, 2, 1});  // scale, then translate
    check_vec((s * t) * Vec4{1, 1, 1, 1}, {22, 2, 2, 1});  // translate, then scale
  }

  TEST_CASE("translation moves points but not directions") {
    Mat4 t = translate({1, 2, 3});
    check_vec(t * Vec4{1, 1, 1, 1}, {2, 3, 4, 1});
    check_vec(t * Vec4{1, 1, 1, 0}, {1, 1, 1, 0});
  }

  TEST_CASE("rotations follow the right-hand rule") {
    const float quarter = 1.5707963f;
    check_vec(rotate_y(quarter) * Vec4{1, 0, 0, 1}, {0, 0, -1, 1});
    check_vec(rotate_y(quarter) * Vec4{0, 0, 1, 1}, {1, 0, 0, 1});
    check_vec(rotate_x(quarter) * Vec4{0, 1, 0, 1}, {0, 0, 1, 1});
  }

  TEST_CASE("perspective maps near to z=0 and far to z=1") {
    const float near = 0.5f, far = 20.0f;
    Mat4 p = perspective(1.0f, 1.5f, near, far);

    Vec4 a = p * Vec4{0, 0, -near, 1};
    CHECK(a.w == Approx(near));
    CHECK(a.z / a.w == Approx(0.0f).epsilon(1e-5));

    Vec4 b = p * Vec4{0, 0, -far, 1};
    CHECK(b.w == Approx(far));
    CHECK(b.z / b.w == Approx(1.0f).epsilon(1e-5));

    Vec4 mid = p * Vec4{0, 0, -5, 1};
    float z = mid.z / mid.w;
    CHECK(z > 0.0f);
    CHECK(z < 1.0f);
  }

  TEST_CASE("perspective: the frustum edge lands on the NDC edge") {
    const float fovy = 1.2f, aspect = 2.0f;
    Mat4 p = perspective(fovy, aspect, 0.1f, 100.0f);
    float t = std::tan(fovy / 2);
    Vec4 top = p * Vec4{0, 3 * t, -3, 1};  // 3 units away, at the top edge
    CHECK(top.y / top.w == Approx(1.0f));
    Vec4 right = p * Vec4{3 * t * aspect, 0, -3, 1};
    CHECK(right.x / right.w == Approx(1.0f));
  }

  TEST_CASE("look_at puts the target straight ahead on -z") {
    Vec3 eye{3, 2, 5}, target{-1, 0, 1};
    Mat4 v = look_at(eye, target, {0, 1, 0});
    Vec4 t = v * point(target);
    CHECK(t.x == Approx(0.0f).epsilon(1e-5));
    CHECK(t.y == Approx(0.0f).epsilon(1e-5));
    CHECK(-t.z == Approx(length(target - eye)));
    check_vec(v * point(eye), {0, 0, 0, 1});
  }
}
