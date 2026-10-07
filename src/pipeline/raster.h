#pragma once

#include <vector>

#include "pipeline/framebuffer.h"
#include "pipeline/shading.h"
#include "pipeline/vertex.h"

// Setup, rasterization with the top-left fill rule, depth test,
// interpolation and shading for one screen-space triangle.
void draw_triangle(Framebuffer& fb, const ScreenVertex& a, const ScreenVertex& b,
                   const ScreenVertex& c, const DrawState& state);

// Coverage only: adds 1 to counts[y * width + x] for every pixel the triangle
// covers. Must follow exactly the same rules as draw_triangle; the fill-rule
// test relies on it. Draws either winding.
void accumulate_coverage(Vec2 a, Vec2 b, Vec2 c, int width, int height, std::vector<int>& counts);
