cmake_minimum_required(VERSION 3.22)
# VERSION is required: juce_add_plugin needs PROJECT_VERSION to derive the
# plugin version, and errors out without it.
project(GranularFreezePlugin VERSION 0.1.0)

# This CMake skeleton expects JUCE to be available as a submodule or installed
# If JUCE is not provided via JUCE_SOURCE_DIR or as a submodule, use CMake FetchContent
# to download a known JUCE release for CI convenience. See docs/CI_SECRETS.md for
# alternate approaches (submodule or preinstalled JUCE).

include(FetchContent)

# Allow the environment or callers to override JUCE_SOURCE_DIR/JUCE_DIR
if(DEFINED JUCE_SOURCE_DIR)
  message(STATUS "Using JUCE from JUCE_SOURCE_DIR=${JUCE_SOURCE_DIR}")
  add_subdirectory(${JUCE_SOURCE_DIR} JUCE)
else()
  message(STATUS "JUCE not provided; fetching JUCE via FetchContent (this may increase CI time)")
  FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    # JUCE 7.0.4 fails to build against the macOS 15+ SDK: it calls
    # CGWindowListCreateImage unconditionally, which that SDK marks as
    # unavailable (a hard error, not a warning). 8.x guards the call behind a
    # ScreenCaptureKit branch, so it compiles on current runners.
    GIT_TAG 8.0.15
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(juce)
endif()

# Plugin target
add_subdirectory(src)
