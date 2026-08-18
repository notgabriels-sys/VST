# No project() call here on purpose: a nested project() resets PROJECT_VERSION,
# which juce_add_plugin requires. This directory inherits the root project.

# Simple CMake target producing a JUCE plugin
# This assumes JUCE is available and that juce_add_plugin is in scope (via JUCE's CMake setup)

if(NOT TARGET juce::juce_audio_processors)
  message(FATAL_ERROR "JUCE CMake not found. Add JUCE as a submodule or set JUCE_DIR and include JUCE's CMake files.")
endif()

juce_add_plugin(GranularFreeze
  VERSION ${PROJECT_VERSION}
  PLUGIN_MANUFACTURER_CODE GFZP
  PLUGIN_CODE GF01
  FORMATS VST3 AU
  # JUCE errors out unless this is decided explicitly. Per JUCE's own guidance:
  # with neither a VST2 nor a VST3 release out in the wild, ship VST3 only and
  # disable VST2 replacement. This must not change once a VST3 is released --
  # flipping it later breaks parameter automation in saved sessions.
  VST3_CAN_REPLACE_VST2 FALSE
  PRODUCT_NAME "Granular Freeze"
  COMPANY_NAME "Gabriel Garcia Alonso"
)

# Sources use `#include <JuceHeader.h>`, which only exists if this is called.
juce_generate_juce_header(GranularFreeze)

target_sources(GranularFreeze PRIVATE
  PluginProcessor.cpp
  PluginProcessor.h
  PluginEditor.cpp
  PluginEditor.h
)

target_link_libraries(GranularFreeze PRIVATE juce::juce_audio_processors juce::juce_audio_utils)
