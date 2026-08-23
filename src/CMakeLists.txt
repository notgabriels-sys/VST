# No project() call here on purpose: a nested project() resets PROJECT_VERSION,
# which juce_add_plugin requires. This directory inherits the root project.

# Simple CMake target producing a JUCE plugin
# This assumes JUCE is available and that juce_add_plugin is in scope (via JUCE's CMake setup)

if(NOT TARGET juce::juce_audio_processors)
  message(FATAL_ERROR "JUCE CMake not found. Set GRANULAR_FREEZE_JUCE_SOURCE_DIR to a JUCE checkout, or allow the root FetchContent setup to provide it.")
endif()

juce_add_plugin(GranularFreeze
  VERSION ${PROJECT_VERSION}
  BUNDLE_ID com.gabrielgarciaalonso.granularfreeze
  PLUGIN_MANUFACTURER_CODE GFZP
  PLUGIN_CODE GF01
  FORMATS VST3 AU
  PRODUCT_NAME "Granular Freeze"
  COMPANY_NAME "Gabriel Garcia Alonso"
)

# JUCE's VST3 wrapper refuses to compile until this is decided explicitly, since
# it permanently fixes parameter IDs for released plugins. Note this is a module
# compile definition, NOT a juce_add_plugin keyword -- passing it to
# juce_add_plugin is silently ignored. PUBLIC so it reaches the generated
# GranularFreeze_VST3 target, which is where the guard actually fires.
#
# Per JUCE's guidance: with neither a VST2 nor a VST3 release published, ship
# VST3 only and disable VST2 replacement. Do not flip this once a VST3 ships --
# it would break automation in sessions saved with the released version.
target_compile_definitions(GranularFreeze PUBLIC JUCE_VST3_CAN_REPLACE_VST2=0)

target_sources(GranularFreeze PRIVATE
  GrainEngine.cpp
  GrainEngine.h
  PluginParameters.cpp
  PluginParameters.h
  PluginProcessor.cpp
  PluginProcessor.h
  PluginEditor.cpp
  PluginEditor.h
)

target_link_libraries(GranularFreeze PRIVATE juce::juce_audio_processors)
