cmake_minimum_required(VERSION 3.20)
project(soft_raster LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Sanitizer selection: "address" (ASan + UBSan), "thread" (TSan), or "none".
# ASan and TSan cannot be combined, so they are separate build directories.
set(SR_SANITIZE "none" CACHE STRING "Sanitizer: none | address | thread")
set_property(CACHE SR_SANITIZE PROPERTY STRINGS none address thread)

add_library(sr_options INTERFACE)
target_compile_options(sr_options INTERFACE
  -Wall -Wextra -Wpedantic
  # Never fuse a*b+c into one FMA behind our back. The scalar and SIMD paths
  # must round identically or golden-image comparisons become flaky.
  -ffp-contract=off)
target_compile_options(sr_options INTERFACE $<$<CONFIG:Release>:-O3>)

if(SR_SANITIZE STREQUAL "address")
  target_compile_options(sr_options INTERFACE
    -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined)
  target_link_options(sr_options INTERFACE -fsanitize=address,undefined)
elseif(SR_SANITIZE STREQUAL "thread")
  target_compile_options(sr_options INTERFACE -fsanitize=thread -fno-omit-frame-pointer)
  target_link_options(sr_options INTERFACE -fsanitize=thread)
elseif(NOT SR_SANITIZE STREQUAL "none")
  message(FATAL_ERROR "Unknown SR_SANITIZE value: ${SR_SANITIZE}")
endif()

# Third-party headers are included as SYSTEM so their warnings stay quiet.
add_library(sr_third_party STATIC src/io/third_party_impl.cpp)
target_include_directories(sr_third_party SYSTEM PUBLIC third_party)
target_compile_options(sr_third_party PRIVATE -w)

# Core renderer: everything except the window. Benchmarks and tests link this.
add_library(sr_core STATIC
  src/io/image.cpp
  src/io/mesh.cpp
  src/io/scenes.cpp
  src/math/mat4.cpp
  src/pipeline/clip.cpp
  src/pipeline/framebuffer.cpp
  src/pipeline/raster.cpp
  src/pipeline/renderer.cpp
  src/pipeline/shading.cpp
  src/pipeline/vertex.cpp)
target_include_directories(sr_core PUBLIC src)
target_link_libraries(sr_core PUBLIC sr_options sr_third_party)

# Viewer + headless renderer.
find_package(SDL2 CONFIG REQUIRED)
add_executable(soft_raster src/app/main.cpp src/app/window.cpp)
target_link_libraries(soft_raster PRIVATE sr_core SDL2::SDL2)

# Benchmark harness.
add_executable(sr_bench bench/bench_main.cpp)
target_link_libraries(sr_bench PRIVATE sr_core)

# Unit and golden-image tests.
enable_testing()
add_executable(sr_tests
  tests/test_main.cpp
  tests/test_image.cpp
  tests/test_mesh.cpp
  tests/test_math.cpp
  tests/test_raster.cpp
  tests/test_clip.cpp
  tests/test_shading.cpp
  tests/test_scenes.cpp)
target_link_libraries(sr_tests PRIVATE sr_core)
target_compile_definitions(sr_tests PRIVATE SR_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
add_test(NAME sr_tests COMMAND sr_tests)
