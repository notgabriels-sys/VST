cmake_minimum_required(VERSION 3.22)
project(GranularFreezePlugin VERSION 0.2.0 LANGUAGES C CXX)
include(FetchContent)
if(DEFINED GRANULAR_FREEZE_DPF_SOURCE_DIR)
  message(STATUS "Using DPF from ${GRANULAR_FREEZE_DPF_SOURCE_DIR}")
  add_subdirectory(${GRANULAR_FREEZE_DPF_SOURCE_DIR} DPF)
  set(GRANULAR_FREEZE_DPF_ROOT "${GRANULAR_FREEZE_DPF_SOURCE_DIR}")
else()
  FetchContent_Declare(dpf
    GIT_REPOSITORY https://github.com/DISTRHO/DPF.git
    GIT_TAG 4238e1c7f0351bbe488d79f0899c540543ac7583
    GIT_SUBMODULES_RECURSE TRUE)
  FetchContent_MakeAvailable(dpf)
  set(GRANULAR_FREEZE_DPF_ROOT "${dpf_SOURCE_DIR}")
endif()

if(MSVC)
  # DPF's own file(DOWNLOAD) does not inspect the transfer status and can leave
  # zero-byte Khronos headers behind. Populate both files from immutable
  # official registry commits and verify their exact bytes before DGL runs.
  file(MAKE_DIRECTORY
    "${GRANULAR_FREEZE_DPF_ROOT}/khronos/GL"
    "${GRANULAR_FREEZE_DPF_ROOT}/khronos/KHR")
  file(DOWNLOAD
    "https://raw.githubusercontent.com/KhronosGroup/OpenGL-Registry/e8f7cd0e35ac8d6f5667a021ff83d04b1fec41ef/api/GL/glext.h"
    "${GRANULAR_FREEZE_DPF_ROOT}/khronos/GL/glext.h"
    EXPECTED_HASH SHA256=d7b6f15b1bb209e08bd6ada36983f8bb40429e66c24083aa2474d55205d97047
    TLS_VERIFY ON)
  file(DOWNLOAD
    "https://raw.githubusercontent.com/KhronosGroup/EGL-Registry/3d7796b3721d93976b6bfe536aa97bbc4bce8667/api/KHR/khrplatform.h"
    "${GRANULAR_FREEZE_DPF_ROOT}/khronos/KHR/khrplatform.h"
    EXPECTED_HASH SHA256=7b1e01aaa7ad8f6fc34b5c7bdf79ebf5189bb09e2c4d2e79fc5d350623d11e83
    TLS_VERIFY ON)
endif()

add_subdirectory(src)
option(GRANULAR_FREEZE_BUILD_TESTS "Build offline behavioural tests" ON)
if(GRANULAR_FREEZE_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
