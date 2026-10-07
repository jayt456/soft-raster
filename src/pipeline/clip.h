#pragma once

#include "pipeline/vertex.h"

// Clipping against one plane adds at most one vertex. Sized for near-plane
// clipping now, with room to add more planes (guard band) later.
constexpr int kMaxClipVertices = 9;

// True when all three vertices are outside the same frustum plane, so no
// part of the triangle can be visible.
bool outside_frustum(const ClipVertex& a, const ClipVertex& b, const ClipVertex& c);

// Clips a triangle against the near plane (z >= 0 in clip space). Writes the
// resulting convex polygon to `out` and returns its vertex count: 0, 3 or 4.
int clip_triangle(const ClipVertex in[3], ClipVertex out[kMaxClipVertices]);
