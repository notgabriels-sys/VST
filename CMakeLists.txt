cmake_minimum_required(VERSION 3.15)
project(GranularFreezePlugin)

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
    # Lock to a stable tag known to work with this project; adjust if needed
    GIT_TAG 7.0.4
  )
  FetchContent_MakeAvailable(juce)
endif()

# Plugin target
add_subdirectory(src)
