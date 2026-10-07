#include "math/mat4.h"

Mat4 operator*(const Mat4& a, const Mat4& b) {
  Mat4 ret;
  for (int col = 0; col < 4; col++) {
    for (int row = 0; row < 4; row++) {
      float sum = 0;
      for (int k = 0; k < 4; ++k) sum += a(row, k) * b(k, col);
      ret(row, col) = sum;
    }
  }
  return ret;
}

Vec4 operator*(const Mat4& a, Vec4 v) {
  return {
      a(0, 0) * v.x + a(0, 1) * v.y + a(0, 2) * v.z + a(0, 3) * v.w,
      a(1, 0) * v.x + a(1, 1) * v.y + a(1, 2) * v.z + a(1, 3) * v.w,
      a(2, 0) * v.x + a(2, 1) * v.y + a(2, 2) * v.z + a(2, 3) * v.w,
      a(3, 0) * v.x + a(3, 1) * v.y + a(3, 2) * v.z + a(3, 3) * v.w,
  };
}

Mat4 translate(Vec3 t) {
  Mat4 r = Mat4::identity();
  r(0, 3) = t.x;
  r(1, 3) = t.y;
  r(2, 3) = t.z;
  return r;
}

Mat4 scale(float s) {
  Mat4 r = Mat4::identity();
  r(0, 0) = r(1, 1) = r(2, 2) = s;
  return r;
}

Mat4 rotate_x(float radians) {
  float c = std::cos(radians), s = std::sin(radians);
  Mat4 r = Mat4::identity();
  r(1, 1) = c;
  r(1, 2) = -s;
  r(2, 1) = s;
  r(2, 2) = c;
  return r;
}

Mat4 rotate_y(float radians) {
  float c = std::cos(radians), s = std::sin(radians);
  Mat4 r = Mat4::identity();
  r(0, 0) = c;
  r(0, 2) = s;
  r(2, 0) = -s;
  r(2, 2) = c;
  return r;
}

Mat4 perspective(float fovy_radians, float aspect, float near, float far) {
  float f = 1.0f / std::tan(fovy_radians * 0.5f);
  Mat4 r;
  r(0, 0) = f / aspect;
  r(1, 1) = f;
  r(2, 2) = far / (near - far);
  r(2, 3) = near * far / (near - far);
  r(3, 2) = -1.0f;  // w_clip = -z_view: distance in front of the camera
  return r;
}

Mat4 look_at(Vec3 eye, Vec3 target, Vec3 up) {
  Vec3 f = normalize(target - eye);  // forward
  Vec3 r = normalize(cross(f, up));  // right
  Vec3 u = cross(r, f);              // true up

  Mat4 m = Mat4::identity();
  m(0, 0) = r.x;  m(0, 1) = r.y;  m(0, 2) = r.z;
  m(1, 0) = u.x;  m(1, 1) = u.y;  m(1, 2) = u.z;
  m(2, 0) = -f.x; m(2, 1) = -f.y; m(2, 2) = -f.z;
  m(0, 3) = -dot(r, eye);
  m(1, 3) = -dot(u, eye);
  m(2, 3) = dot(f, eye);
  return m;
}
