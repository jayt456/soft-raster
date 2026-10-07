// Benchmark harness: renders a fixed scene headless, times every frame, and
// appends one summary row to a CSV (default results/results.csv).
//
// Frame N always shows the same camera position, so runs are comparable
// across commits. The row also records the machine, the compiler, the power
// source and whether the working tree had uncommitted changes, so every
// number can be traced back to exactly what produced it.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <string>
#include <vector>

#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif

#include "io/scenes.h"
#include "pipeline/renderer.h"

namespace {

std::string run_command(const char* cmd) {
  std::string out;
  if (FILE* p = popen(cmd, "r")) {
    char buf[256];
    while (std::fgets(buf, sizeof(buf), p)) out += buf;
    pclose(p);
  }
  while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
  return out;
}

#if defined(__APPLE__)
std::string sysctl_string(const char* name) {
  char buf[256] = {};
  size_t len = sizeof(buf);
  return sysctlbyname(name, buf, &len, nullptr, 0) == 0 ? std::string(buf) : "unknown";
}
int sysctl_int(const char* name) {
  int v = 0;
  size_t len = sizeof(v);
  return sysctlbyname(name, &v, &len, nullptr, 0) == 0 ? v : 0;
}
#endif

struct MachineInfo {
  std::string cpu = "unknown";
  int perf_cores = 0, eff_cores = 0;
  std::string power = "unknown";
};

MachineInfo machine_info() {
  MachineInfo m;
#if defined(__APPLE__)
  m.cpu = sysctl_string("machdep.cpu.brand_string");
  m.perf_cores = sysctl_int("hw.perflevel0.physicalcpu");
  m.eff_cores = sysctl_int("hw.perflevel1.physicalcpu");
  std::string batt = run_command("pmset -g batt 2>/dev/null");
  if (batt.find("AC Power") != std::string::npos) m.power = "AC";
  else if (batt.find("Battery Power") != std::string::npos) m.power = "battery";
#endif
  return m;
}

bool sanitizers_enabled() {
#if defined(__has_feature)
#if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
  return true;
#endif
#endif
  return false;
}

double median(std::vector<double> v) {
  if (v.empty()) return 0;
  std::sort(v.begin(), v.end());
  size_t n = v.size();
  return n % 2 ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}

// Nearest-rank percentile: the smallest sample with at least p% of samples <= it.
double percentile(std::vector<double> v, double p) {
  if (v.empty()) return 0;
  std::sort(v.begin(), v.end());
  size_t rank = static_cast<size_t>(std::ceil(p / 100.0 * v.size()));
  return v[std::clamp<size_t>(rank, 1, v.size()) - 1];
}

std::string csv_quote(const std::string& s) {
  std::string out = "\"";
  for (char c : s) out += (c == '"') ? std::string("\"\"") : std::string(1, c);
  return out + "\"";
}

void usage() {
  std::printf(
      "usage: sr_bench [options]\n"
      "  --scene NAME     any scene soft_raster accepts (default: cube)\n"
      "  --texture PATH   texture image\n"
      "  --size WxH       resolution (default: 1280x720)\n"
      "  --frames N       timed frames (default: 200)\n"
      "  --warmup N       untimed warm-up frames (default: 20)\n"
      "  --config NAME    label for this configuration, e.g. scalar, neon, tiles-8t (default: scalar)\n"
      "  --csv PATH       results file to append to (default: results/results.csv)\n"
      "  --no-csv         print only, do not append\n");
}

}  // namespace

int main(int argc, char** argv) {
  std::string scene_name = "cube", texture_path, config = "scalar", csv_path = "results/results.csv";
  int width = 1280, height = 720, frames = 200, warmup = 20;
  bool write_csv = true;

  for (int i = 1; i < argc; ++i) {
    auto next = [&]() -> const char* {
      if (i + 1 >= argc) {
        std::fprintf(stderr, "missing value for %s\n", argv[i]);
        std::exit(2);
      }
      return argv[++i];
    };
    if (!std::strcmp(argv[i], "--scene")) scene_name = next();
    else if (!std::strcmp(argv[i], "--texture")) texture_path = next();
    else if (!std::strcmp(argv[i], "--frames")) frames = std::atoi(next());
    else if (!std::strcmp(argv[i], "--warmup")) warmup = std::atoi(next());
    else if (!std::strcmp(argv[i], "--config")) config = next();
    else if (!std::strcmp(argv[i], "--csv")) csv_path = next();
    else if (!std::strcmp(argv[i], "--no-csv")) write_csv = false;
    else if (!std::strcmp(argv[i], "--size")) {
      if (std::sscanf(next(), "%dx%d", &width, &height) != 2 || width <= 0 || height <= 0) {
        std::fprintf(stderr, "bad --size, expected WxH\n");
        return 2;
      }
    } else {
      usage();
      return std::strcmp(argv[i], "--help") ? 2 : 0;
    }
  }
  if (frames < 1) {
    std::fprintf(stderr, "--frames must be >= 1\n");
    return 2;
  }

  std::string error;
  auto scene = build_scene(scene_name, texture_path, &error);
  if (!scene) {
    std::fprintf(stderr, "%s\n", error.c_str());
    return 1;
  }

#ifndef NDEBUG
  std::fprintf(stderr, "WARNING: assertions enabled (not a Release build). Numbers are not comparable.\n");
#endif
  if (sanitizers_enabled())
    std::fprintf(stderr, "WARNING: sanitizers enabled. Numbers are not comparable.\n");

  Framebuffer fb(width, height);
  Renderer renderer;
  for (int f = 0; f < warmup; ++f) renderer.render(fb, *scene, f);

  std::vector<double> total, vertex, raster;
  total.reserve(frames);
  for (int f = 0; f < frames; ++f) {
    auto t0 = std::chrono::steady_clock::now();
    FrameStats st = renderer.render(fb, *scene, warmup + f);
    auto t1 = std::chrono::steady_clock::now();
    total.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
    vertex.push_back(st.vertex_ms);
    raster.push_back(st.raster_ms);
  }

  MachineInfo m = machine_info();
  std::string commit = run_command("git rev-parse --short HEAD 2>/dev/null");
  if (commit.empty()) commit = "none";
  bool dirty = !run_command("git status --porcelain --untracked-files=no 2>/dev/null").empty();
  if (dirty) std::fprintf(stderr, "WARNING: uncommitted changes; this row is tagged dirty.\n");
  if (m.power == "battery") std::fprintf(stderr, "WARNING: running on battery power.\n");

  double med = median(total), p95 = percentile(total, 95);
  std::printf("%s | %s | %dx%d | %zu tris | %d frames\n", config.c_str(), scene_name.c_str(), width,
              height, scene->triangle_count(), frames);
  std::printf("  frame   median %8.3f ms   p95 %8.3f ms   (%.1f fps at median)\n", med, p95,
              med > 0 ? 1000.0 / med : 0.0);
  std::printf("  stages  vertex %8.3f ms   clip+raster+shade %8.3f ms (medians)\n", median(vertex),
              median(raster));
  std::printf("  %s, %dP+%dE cores, power: %s, commit %s%s\n", m.cpu.c_str(), m.perf_cores,
              m.eff_cores, m.power.c_str(), commit.c_str(), dirty ? " (dirty)" : "");

  if (!write_csv) return 0;

  std::filesystem::path csv(csv_path);
  if (csv.has_parent_path()) std::filesystem::create_directories(csv.parent_path());
  bool new_file = !std::filesystem::exists(csv);
  FILE* out = std::fopen(csv_path.c_str(), "a");
  if (!out) {
    std::fprintf(stderr, "could not open %s\n", csv_path.c_str());
    return 1;
  }
  if (new_file) {
    std::fprintf(out,
                 "timestamp,commit,dirty,scene,triangles,width,height,config,frames,"
                 "median_ms,p95_ms,vertex_ms,raster_ms,"
                 "cpu,perf_cores,eff_cores,power,compiler\n");
  }
  std::time_t now = std::time(nullptr);
  char ts[32];
  std::strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%S", std::localtime(&now));
  std::fprintf(out, "%s,%s,%d,%s,%zu,%d,%d,%s,%d,%.4f,%.4f,%.4f,%.4f,%s,%d,%d,%s,%s\n", ts,
               commit.c_str(), dirty ? 1 : 0, csv_quote(scene_name).c_str(),
               scene->triangle_count(), width, height, csv_quote(config).c_str(), frames, med,
               p95, median(vertex), median(raster), csv_quote(m.cpu).c_str(),
               m.perf_cores, m.eff_cores, m.power.c_str(), csv_quote(__VERSION__).c_str());
  std::fclose(out);
  std::printf("  appended to %s\n", csv_path.c_str());
  return 0;
}
