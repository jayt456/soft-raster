#include "io/mesh.h"

#include <algorithm>
#include <map>
#include <tuple>

#include "tiny_obj_loader.h"

std::optional<Mesh> load_obj(const std::string& path, std::string* error) {
  tinyobj::ObjReaderConfig config;
  config.triangulate = true;
  config.vertex_color = false;

  tinyobj::ObjReader reader;
  if (!reader.ParseFromFile(path, config)) {
    if (error) *error = reader.Error().empty() ? "failed to parse " + path : reader.Error();
    return std::nullopt;
  }

  const tinyobj::attrib_t& attrib = reader.GetAttrib();
  const bool want_uvs = !attrib.texcoords.empty();
  const bool want_normals = !attrib.normals.empty();

  Mesh mesh;
  std::map<std::tuple<int, int, int>, uint32_t> unique;

  for (const tinyobj::shape_t& shape : reader.GetShapes()) {
    for (const tinyobj::index_t& idx : shape.mesh.indices) {
      auto key = std::make_tuple(idx.vertex_index, idx.texcoord_index, idx.normal_index);
      auto it = unique.find(key);
      if (it != unique.end()) {
        mesh.indices.push_back(it->second);
        continue;
      }

      uint32_t new_index = static_cast<uint32_t>(mesh.positions.size());
      unique.emplace(key, new_index);
      mesh.indices.push_back(new_index);

      const float* p = &attrib.vertices[3 * static_cast<size_t>(idx.vertex_index)];
      mesh.positions.push_back({p[0], p[1], p[2]});

      if (want_uvs) {
        if (idx.texcoord_index >= 0) {
          const float* t = &attrib.texcoords[2 * static_cast<size_t>(idx.texcoord_index)];
          mesh.uvs.push_back({t[0], t[1]});
        } else {
          mesh.uvs.push_back({0, 0});
        }
      }
      if (want_normals) {
        if (idx.normal_index >= 0) {
          const float* n = &attrib.normals[3 * static_cast<size_t>(idx.normal_index)];
          mesh.normals.push_back({n[0], n[1], n[2]});
        } else {
          mesh.normals.push_back({0, 0, 0});
        }
      }
    }
  }

  if (mesh.indices.empty()) {
    if (error) *error = "no triangles in " + path;
    return std::nullopt;
  }
  return mesh;
}

void compute_vertex_normals(Mesh& mesh) {
  mesh.normals.assign(mesh.positions.size(), Vec3{});
  for (size_t t = 0; t + 2 < mesh.indices.size(); t += 3) {
    uint32_t i0 = mesh.indices[t], i1 = mesh.indices[t + 1], i2 = mesh.indices[t + 2];
    Vec3 p0 = mesh.positions[i0], p1 = mesh.positions[i1], p2 = mesh.positions[i2];
    // Unnormalised cross product: its length is twice the triangle area,
    // so larger triangles contribute more to the vertex normal.
    Vec3 n = cross(p1 - p0, p2 - p0);
    mesh.normals[i0] = mesh.normals[i0] + n;
    mesh.normals[i1] = mesh.normals[i1] + n;
    mesh.normals[i2] = mesh.normals[i2] + n;
  }
  for (Vec3& n : mesh.normals) n = normalize(n);
}

void fit_to_unit_size(Mesh& mesh) {
  if (mesh.positions.empty()) return;
  Vec3 lo = mesh.positions[0], hi = lo;
  for (Vec3 p : mesh.positions) {
    lo = {std::min(lo.x, p.x), std::min(lo.y, p.y), std::min(lo.z, p.z)};
    hi = {std::max(hi.x, p.x), std::max(hi.y, p.y), std::max(hi.z, p.z)};
  }
  Vec3 center = (lo + hi) * 0.5f;
  float extent = std::max({hi.x - lo.x, hi.y - lo.y, hi.z - lo.z});
  float s = extent > 0 ? 1.0f / extent : 1.0f;
  for (Vec3& p : mesh.positions) p = (p - center) * s;
}

void generate_spherical_uvs(Mesh& mesh) {
  constexpr float kPi = 3.14159265f;
  mesh.uvs.resize(mesh.positions.size());
  for (size_t i = 0; i < mesh.positions.size(); ++i) {
    Vec3 d = normalize(mesh.positions[i]);
    mesh.uvs[i] = {0.5f + std::atan2(d.z, d.x) / (2 * kPi), 0.5f + std::asin(std::clamp(d.y, -1.0f, 1.0f)) / kPi};
  }
}

Mesh make_cube() {
  struct Face {
    Vec3 n, u, v;  // u x v == n, so corners listed in (u, v) order are CCW from outside
  };
  const Face faces[6] = {
      {{1, 0, 0}, {0, 0, -1}, {0, 1, 0}},  {{-1, 0, 0}, {0, 0, 1}, {0, 1, 0}},
      {{0, 1, 0}, {1, 0, 0}, {0, 0, -1}},  {{0, -1, 0}, {1, 0, 0}, {0, 0, 1}},
      {{0, 0, 1}, {1, 0, 0}, {0, 1, 0}},   {{0, 0, -1}, {-1, 0, 0}, {0, 1, 0}},
  };
  const float su[4] = {-1, 1, 1, -1};
  const float sv[4] = {-1, -1, 1, 1};

  Mesh mesh;
  for (const Face& f : faces) {
    uint32_t base = static_cast<uint32_t>(mesh.positions.size());
    for (int c = 0; c < 4; ++c) {
      mesh.positions.push_back(f.n * 0.5f + f.u * (0.5f * su[c]) + f.v * (0.5f * sv[c]));
      mesh.uvs.push_back({(su[c] + 1) * 0.5f, (sv[c] + 1) * 0.5f});
      mesh.normals.push_back(f.n);
    }
    for (uint32_t i : {0u, 1u, 2u, 0u, 2u, 3u}) mesh.indices.push_back(base + i);
  }
  return mesh;
}

Mesh make_grid_plane(int n, float half, float uv_scale) {
  Mesh mesh;
  for (int j = 0; j <= n; ++j) {
    for (int i = 0; i <= n; ++i) {
      float fu = static_cast<float>(i) / n, fv = static_cast<float>(j) / n;
      // x grows with i, z shrinks with j: (+x) x (-z) = +y, so CCW seen from above.
      mesh.positions.push_back({-half + 2 * half * fu, 0, half - 2 * half * fv});
      mesh.uvs.push_back({fu * uv_scale, fv * uv_scale});
      mesh.normals.push_back({0, 1, 0});
    }
  }
  auto idx = [n](int i, int j) { return static_cast<uint32_t>(j * (n + 1) + i); };
  for (int j = 0; j < n; ++j) {
    for (int i = 0; i < n; ++i) {
      uint32_t a = idx(i, j), b = idx(i + 1, j), c = idx(i + 1, j + 1), d = idx(i, j + 1);
      mesh.indices.insert(mesh.indices.end(), {a, b, c, a, c, d});
    }
  }
  return mesh;
}
