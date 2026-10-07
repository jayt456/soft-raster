#include "pipeline/vertex.h"

void transform_vertices(const Mesh& mesh, const Mat4& model, const Mat4& view_proj,
                        std::vector<ClipVertex>& out) {
  const Mat4 mvp = view_proj * model;
  const size_t n = mesh.positions.size();
  out.resize(n);

  for (size_t i = 0; i < n; ++i) {
    ClipVertex& v = out[i];
    v.pos = mvp * point(mesh.positions[i]);
    v.uv = mesh.has_uvs() ? mesh.uvs[i] : Vec2{};
    v.normal = mesh.has_normals() ? xyz(model * direction(mesh.normals[i])) : Vec3{};
  }
}

ScreenVertex to_screen(const ClipVertex& v, int width, int height) {
  const float inv_w = 1.0f / v.pos.w;
  const float ndc_x = v.pos.x * inv_w;
  const float ndc_y = v.pos.y * inv_w;

  ScreenVertex s;
  s.pos = {(ndc_x + 1.0f) * 0.5f * float(width), (1.0f - ndc_y) * 0.5f * float(height)};
  s.z = v.pos.z * inv_w;
  s.inv_w = inv_w;
  s.uv = v.uv;
  s.normal = v.normal;
  return s;
}
