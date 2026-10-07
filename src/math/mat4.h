#pragma once

#include "math/vec.h"

// Column-major 4x4 matrix: element (row, col) lives at m[col * 4 + row].
// Vectors are columns, so `A * B * v` applies B first.
struct Mat4 {
  float m[16] = {};

  float& operator()(int row, int col) { return m[col * 4 + row]; }
  float operator()(int row, int col) const { return m[col * 4 + row]; }

  static Mat4 identity() {
    Mat4 r;
    r(0, 0) = r(1, 1) = r(2, 2) = r(3, 3) = 1.0f;
    return r;
  }
};

Mat4 operator*(const Mat4& a, const Mat4& b);
Vec4 operator*(const Mat4& a, Vec4 v);

Mat4 translate(Vec3 t);
Mat4 scale(float s);
Mat4 rotate_x(float radians);
Mat4 rotate_y(float radians);

// Right-handed view space: the camera looks down -z.
// Maps view-space depth [-near, -far] to NDC z in [0, 1] (D3D/Vulkan/Metal convention).
Mat4 perspective(float fovy_radians, float aspect, float near, float far);

Mat4 look_at(Vec3 eye, Vec3 target, Vec3 up);
