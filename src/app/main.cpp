#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

#include "app/window.h"
#include "io/scenes.h"

static void usage() {
  std::printf(
      "usage: soft_raster [options]\n"
      "  --scene NAME      triangles | cube | intersect | inside | plane | bunny | spot | file.obj\n"
      "  --texture PATH    override the scene's texture\n"
      "  --size WxH        resolution (default 1280x720)\n"
      "  --affine          disable perspective-correct interpolation\n"
      "  --no-cull         disable back-face culling\n"
      "  --no-lighting     texture only\n"
      "  --headless        render one frame to a PNG instead of opening a window\n"
      "  --frame N         headless: which frame to render (default 0)\n"
      "  --out PATH        headless: output file (default out/frame.png)\n");
}

int main(int argc, char** argv) {
  std::string scene_name = "cube", texture_path, out_path = "out/frame.png";
  int width = 1280, height = 720, frame = 0;
  bool headless = false;
  Renderer renderer;

  for (int i = 1; i < argc; ++i) {
    auto next = [&]() -> const char* {
      if (i + 1 >= argc) {
        std::fprintf(stderr, "missing value for %s\n", argv[i]);
        std::exit(2);
      }
      return argv[++i];
    };
    const char* arg = argv[i];
    if (!std::strcmp(arg, "--scene")) scene_name = next();
    else if (!std::strcmp(arg, "--texture")) texture_path = next();
    else if (!std::strcmp(arg, "--affine")) renderer.options.perspective_correct = false;
    else if (!std::strcmp(arg, "--no-cull")) renderer.options.cull_backfaces = false;
    else if (!std::strcmp(arg, "--no-lighting")) renderer.options.lighting = false;
    else if (!std::strcmp(arg, "--headless")) headless = true;
    else if (!std::strcmp(arg, "--frame")) frame = std::atoi(next());
    else if (!std::strcmp(arg, "--out")) out_path = next();
    else if (!std::strcmp(arg, "--size")) {
      if (std::sscanf(next(), "%dx%d", &width, &height) != 2 || width <= 0 || height <= 0) {
        std::fprintf(stderr, "bad --size, expected WxH\n");
        return 2;
      }
    } else {
      usage();
      return std::strcmp(arg, "--help") ? 2 : 0;
    }
  }

  std::string error;
  auto scene = build_scene(scene_name, texture_path, &error);
  if (!scene) {
    std::fprintf(stderr, "%s\n", error.c_str());
    return 1;
  }
  std::printf("%s: %zu triangles\n", scene_name.c_str(), scene->triangle_count());

  if (!headless) return run_window(*scene, renderer, width, height);

  Framebuffer fb(width, height);
  renderer.render(fb, *scene, frame);

  std::filesystem::path out(out_path);
  if (out.has_parent_path()) std::filesystem::create_directories(out.parent_path());
  if (!write_png(out_path, width, height, fb.color.data())) {
    std::fprintf(stderr, "failed to write %s\n", out_path.c_str());
    return 1;
  }
  std::printf("wrote %s\n", out_path.c_str());
  return 0;
}
