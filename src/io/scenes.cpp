#include "io/scenes.h"

#include <cmath>

namespace {

Image checkerboard() {
  return make_checkerboard(256, 8, pack_rgba(230, 230, 230), pack_rgba(60, 60, 70));
}

Scene flat_triangles() {
  Scene s;
  s.double_sided = true;
  const uint32_t palette[] = {
      pack_rgba(230, 80, 70),  pack_rgba(240, 170, 60), pack_rgba(230, 220, 90),
      pack_rgba(110, 200, 110), pack_rgba(80, 180, 220), pack_rgba(110, 120, 230),
      pack_rgba(180, 100, 220), pack_rgba(230, 110, 170),
  };
  auto add = [&s](Vec2 a, Vec2 b, Vec2 c, uint32_t color) {
    for (Vec2 p : {a, b, c}) {
      ScreenVertex v;
      v.pos = p;
      s.screen_triangles.push_back(v);
    }
    s.screen_colors.push_back(color);
  };

  // A fan: every edge but the outer ones is shared by two triangles.
  const Vec2 center{400.3f, 360.6f};
  const int kSlices = 8;
  for (int i = 0; i < kSlices; ++i) {
    float a0 = 2 * 3.14159265f * float(i) / kSlices, a1 = 2 * 3.14159265f * float(i + 1) / kSlices;
    Vec2 p0{center.x + 250 * std::cos(a0), center.y + 250 * std::sin(a0)};
    Vec2 p1{center.x + 250 * std::cos(a1), center.y + 250 * std::sin(a1)};
    add(center, p0, p1, palette[i]);
  }

  // Edges exactly through pixel centers: a square split along its diagonal.
  add({760.5f, 120.5f}, {1060.5f, 120.5f}, {1060.5f, 420.5f}, palette[1]);
  add({760.5f, 120.5f}, {1060.5f, 420.5f}, {760.5f, 420.5f}, palette[4]);

  // A long thin sliver.
  add({720.0f, 520.0f}, {1220.0f, 560.0f}, {720.0f, 530.0f}, palette[6]);
  return s;
}

Scene cube() {
  Scene s;
  s.mesh = make_cube();
  s.texture = checkerboard();
  s.model = rotate_x(0.45f);
  s.spin = 0.012f;
  s.camera.eye = {0, 0.4f, 2.4f};
  return s;
}

Scene intersect() {
  Scene s;
  s.mesh.positions = {{-0.8f, -0.5f, 0}, {0.8f, -0.5f, 0},  {0, 0.7f, 0},
                      {-0.6f, 0, -0.7f}, {0.7f, 0.1f, 0.6f}, {0, 0.6f, -0.4f}};
  s.mesh.uvs = {{0, 0}, {1, 0}, {0.5f, 1}, {0, 0}, {1, 0}, {0.5f, 1}};
  s.mesh.indices = {0, 1, 2, 3, 4, 5};
  compute_vertex_normals(s.mesh);
  s.texture = checkerboard();
  s.spin = 0.01f;
  s.double_sided = true;
  s.camera.eye = {0, 0.3f, 2.2f};
  return s;
}

Scene inside() {
  Scene s;
  s.mesh = make_cube();
  s.texture = checkerboard();
  s.double_sided = true;
  s.camera.eye = {0.1f, 0, 0.45f};  // 5 cm from the +z wall, walls cross the near plane
  s.camera.target = {-0.3f, -0.1f, -1};
  s.ambient = 0.4f;
  return s;
}

Scene plane() {
  Scene s;
  s.mesh = make_grid_plane(1, 50.0f, 50.0f);
  s.texture = checkerboard();
  s.camera.eye = {0, 0.6f, 4};
  s.camera.target = {0, 0, -10};
  return s;
}

std::optional<Scene> model(const std::string& path, std::string* error) {
  auto mesh = load_obj(path, error);
  if (!mesh) return std::nullopt;
  Scene s;
  s.mesh = std::move(*mesh);
  fit_to_unit_size(s.mesh);
  if (!s.mesh.has_normals()) compute_vertex_normals(s.mesh);
  if (!s.mesh.has_uvs()) {
    generate_spherical_uvs(s.mesh);
    s.texture = checkerboard();
  }
  s.spin = 0.01f;
  s.camera.eye = {0, 0.25f, 1.6f};
  return s;
}

}  // namespace

std::optional<Scene> build_scene(const std::string& name, const std::string& texture_path,
                                 std::string* error) {
  std::optional<Scene> scene;
  if (name == "triangles") scene = flat_triangles();
  else if (name == "cube") scene = cube();
  else if (name == "intersect") scene = intersect();
  else if (name == "inside") scene = inside();
  else if (name == "plane") scene = plane();
  else if (name == "bunny") scene = model("assets/bunny.obj", error);
  else if (name == "spot") {
    scene = model("assets/spot.obj", error);
    if (scene && texture_path.empty()) {
      auto tex = load_image("assets/spot_texture.png");
      if (tex) scene->texture = std::move(*tex);
    }
  } else if (name.ends_with(".obj")) {
    scene = model(name, error);
  } else {
    if (error) *error = "unknown scene '" + name + "'";
    return std::nullopt;
  }
  if (!scene) return std::nullopt;

  if (!texture_path.empty()) {
    auto tex = load_image(texture_path);
    if (!tex) {
      if (error) *error = "could not load texture " + texture_path;
      return std::nullopt;
    }
    scene->texture = std::move(*tex);
  }
  return scene;
}
