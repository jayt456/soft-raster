#include "pipeline/clip.h"

namespace {

  // One bit per frustum plane the point is outside of.
  unsigned outcode(Vec4 p) {
    unsigned code = 0;
    if (p.x < -p.w) code |= 1;   // left
    if (p.x > p.w) code |= 2;    // right
    if (p.y < -p.w) code |= 4;   // bottom
    if (p.y > p.w) code |= 8;    // top
    if (p.z < 0) code |= 16;     // near
    if (p.z > p.w) code |= 32;   // far
    return code;
  }

  // The point a fraction t of the way from a to b, with every attribute blended.
  ClipVertex intersect(const ClipVertex& a, const ClipVertex& b, float t) {
    ClipVertex r;
    r.pos = lerp(a.pos, b.pos, t);
    r.uv = lerp(a.uv, b.uv, t);
    r.normal = lerp(a.normal, b.normal, t);
    return r;
  }

}  // namespace

bool outside_frustum(const ClipVertex& a, const ClipVertex& b, const ClipVertex& c) {
  return (outcode(a.pos) & outcode(b.pos) & outcode(c.pos)) != 0;
}

int clip_triangle(const ClipVertex in[3], ClipVertex out[kMaxClipVertices]) {
  int n = 0;
  for (int i = 0; i < 3; ++i) {
    const ClipVertex& cur = in[i];
    const ClipVertex& next = in[(i + 1) % 3];
    float d_cur = cur.pos.z, d_next = next.pos.z;  // signed distance to the near plane

    if (d_cur >= 0) out[n++] = cur;
    if ((d_cur >= 0) != (d_next >= 0)) out[n++] = intersect(cur, next, d_cur / (d_cur - d_next));
  }
  return n >= 3 ? n : 0;
}
