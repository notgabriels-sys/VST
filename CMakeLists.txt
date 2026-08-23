cmake_minimum_required(VERSION 3.22)
# VERSION is required: juce_add_plugin needs PROJECT_VERSION to derive the
# plugin version, and errors out without it.
project(GranularFreezePlugin VERSION 0.2.0)

# Callers may provide a local JUCE checkout through
# GRANULAR_FREEZE_JUCE_SOURCE_DIR. Otherwise, use FetchContent below to fetch
# the pinned JUCE release.

include(FetchContent)

# Use a project-specific override name. FetchContent reserves and populates
# JUCE_SOURCE_DIR internally, so treating that variable as user input makes a
# second configure mistake the cached download for an external checkout.
if(DEFINED GRANULAR_FREEZE_JUCE_SOURCE_DIR)
  message(STATUS "Using JUCE from GRANULAR_FREEZE_JUCE_SOURCE_DIR=${GRANULAR_FREEZE_JUCE_SOURCE_DIR}")
  add_subdirectory(${GRANULAR_FREEZE_JUCE_SOURCE_DIR} JUCE)
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

# Offline behavioural tests and the optional render utility.
option(GRANULAR_FREEZE_BUILD_TESTS "Build offline behavioural tests" ON)
if(GRANULAR_FREEZE_BUILD_TESTS)
  add_subdirectory(tests)
endif()
