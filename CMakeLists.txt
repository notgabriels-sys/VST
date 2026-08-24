cmake_minimum_required(VERSION 3.22)
project(GranularFreezePlugin VERSION 0.2.0 LANGUAGES C CXX)
include(FetchContent)
if(DEFINED GRANULAR_FREEZE_DPF_SOURCE_DIR)
  message(STATUS "Using DPF from ${GRANULAR_FREEZE_DPF_SOURCE_DIR}")
  add_subdirectory(${GRANULAR_FREEZE_DPF_SOURCE_DIR} DPF)
else()
  FetchContent_Declare(dpf
    GIT_REPOSITORY https://github.com/DISTRHO/DPF.git
    GIT_TAG 4238e1c7f0351bbe488d79f0899c540543ac7583
    GIT_SUBMODULES_RECURSE TRUE)
  FetchContent_MakeAvailable(dpf)
endif()
add_subdirectory(src)
option(GRANULAR_FREEZE_BUILD_TESTS "Build offline behavioural tests" ON)
if(GRANULAR_FREEZE_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
