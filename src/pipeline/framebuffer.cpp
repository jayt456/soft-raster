#include "pipeline/framebuffer.h"

#include <algorithm>

Framebuffer::Framebuffer(int w, int h)
    : width(w),
      height(h),
      color(static_cast<size_t>(w) * h),
      depth(static_cast<size_t>(w) * h) {}

void Framebuffer::clear(uint32_t rgba, float depth_value) {
  std::fill(color.begin(), color.end(), rgba);
  std::fill(depth.begin(), depth.end(), depth_value);
}
