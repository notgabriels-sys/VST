cmake_minimum_required(VERSION 3.15)
project(GranularFreezePlugin)

# This CMake skeleton expects JUCE to be available as a submodule or installed
# See docs/CI_SECRETS.md for instructions on providing JUCE in CI.

# Option A: set JUCE_SOURCE_DIR to the path of JUCE
if(NOT DEFINED JUCE_SOURCE_DIR)
  message(WARNING "JUCE_SOURCE_DIR not set — configure JUCE as submodule or provide JUCE_DIR in your environment")
endif()

# Minimal configuration — user should include JUCE via add_subdirectory(JUCE) or use juce
auto_add_juce_library()

# Plugin target
add_subdirectory(src)
