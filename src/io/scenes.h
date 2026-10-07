#pragma once

#include <optional>
#include <string>

#include "pipeline/scene.h"

// Named scenes shared by the viewer, the headless renderer, the benchmark
// and the tests, so they all draw exactly the same thing.
//
//   triangles  M1: flat 2D triangles in pixel coordinates (1280x720 layout)
//   cube       M2: spinning textured cube
//   intersect  M2: two triangles passing through each other (depth test)
//   inside     M2: camera inside a cube, close to a wall (near-plane clipping)
//   plane      M3: checkerboard ground plane at a grazing angle (perspective)
//   bunny      M3: Stanford bunny, 69K triangles, generated UVs
//   spot       M3: Keenan Crane's cow with its own texture
//   path.obj   any OBJ file
std::optional<Scene> build_scene(const std::string& name, const std::string& texture_path = "",
                                 std::string* error = nullptr);
