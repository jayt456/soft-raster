#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "math/vec.h"

// Indexed triangle mesh in structure-of-arrays form. Every vertex has a
// position; uvs and normals are either empty or the same length as positions.
// Triangles are counter-clockwise when viewed from the front.
struct Mesh {
  std::vector<Vec3> positions;
  std::vector<Vec2> uvs;
  std::vector<Vec3> normals;
  std::vector<uint32_t> indices;  // 3 per triangle

  size_t triangle_count() const { return indices.size() / 3; }
  bool has_uvs() const { return !uvs.empty(); }
  bool has_normals() const { return !normals.empty(); }
};

// Loads a triangulated OBJ. Vertices sharing the same (position, uv, normal)
// index triple are merged. Returns nullopt and fills `error` on failure.
std::optional<Mesh> load_obj(const std::string& path, std::string* error = nullptr);

// Area-weighted smooth vertex normals, for meshes that ship without them
// (the Stanford bunny, for example).
void compute_vertex_normals(Mesh& mesh);

// Centers the mesh on the origin and scales it so its largest dimension is 1.
void fit_to_unit_size(Mesh& mesh);

// Spherical-projection UVs around the origin, for models without UVs.
// There is a visible seam where u wraps from 1 back to 0.
void generate_spherical_uvs(Mesh& mesh);

// Unit cube centred on the origin, 24 vertices so each face has its own
// uvs (0..1 per face) and flat normals.
Mesh make_cube();

// Flat square in the XZ plane from -half to +half, subdivided into
// `n` x `n` cells (2 * n * n triangles), uvs 0..uv_scale, normal +Y.
// Many shared edges, so it doubles as the fill-rule test mesh.
Mesh make_grid_plane(int n, float half, float uv_scale = 1.0f);
