#include <filesystem>
#include <fstream>

#include "doctest.h"
#include "io/mesh.h"

TEST_SUITE_BEGIN("framework");

// Triangle winding is CCW seen from the side its geometric normal points to,
// so the geometric normal must agree with the stored vertex normal.
static void check_winding_matches_normals(const Mesh& m) {
  for (size_t t = 0; t < m.triangle_count(); ++t) {
    uint32_t i0 = m.indices[3 * t], i1 = m.indices[3 * t + 1], i2 = m.indices[3 * t + 2];
    Vec3 geometric = cross(m.positions[i1] - m.positions[i0], m.positions[i2] - m.positions[i0]);
    CHECK(dot(geometric, m.normals[i0]) > 0);
  }
}

TEST_CASE("cube: 12 triangles, CCW from outside, normals point outward") {
  Mesh m = make_cube();
  CHECK(m.triangle_count() == 12);
  CHECK(m.positions.size() == 24);
  CHECK(m.uvs.size() == 24);
  check_winding_matches_normals(m);
  for (size_t i = 0; i < m.positions.size(); ++i) CHECK(dot(m.positions[i], m.normals[i]) > 0);
}

TEST_CASE("grid plane: 2*n*n triangles, CCW seen from +Y") {
  Mesh m = make_grid_plane(5, 1.0f);
  CHECK(m.triangle_count() == 50);
  CHECK(m.positions.size() == 36);
  check_winding_matches_normals(m);
  for (Vec3 n : m.normals) CHECK(n.y == 1.0f);
}

TEST_CASE("OBJ loader merges shared vertices and keeps uvs/normals") {
  auto path = std::filesystem::temp_directory_path() / "sr_quad.obj";
  {
    std::ofstream f(path);
    f << "v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\n"
         "vt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\n"
         "vn 0 0 1\n"
         "f 1/1/1 2/2/1 3/3/1 4/4/1\n";  // a quad, triangulated by the loader
  }
  std::string error;
  auto m = load_obj(path.string(), &error);
  REQUIRE_MESSAGE(m.has_value(), error);
  CHECK(m->triangle_count() == 2);
  CHECK(m->positions.size() == 4);  // the diagonal's two vertices are shared
  CHECK(m->uvs.size() == 4);
  CHECK(m->normals.size() == 4);
  check_winding_matches_normals(*m);
  std::filesystem::remove(path);
}

TEST_CASE("compute_vertex_normals on a mesh without normals") {
  Mesh m = make_cube();
  m.normals.clear();
  compute_vertex_normals(m);
  REQUIRE(m.normals.size() == m.positions.size());
  // Cube faces don't share vertices, so each vertex gets its face normal back.
  for (size_t i = 0; i < m.positions.size(); ++i) CHECK(doctest::Approx(length(m.normals[i])) == 1.0f);
  check_winding_matches_normals(m);
}

TEST_SUITE_END();
