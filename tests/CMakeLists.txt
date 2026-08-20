# Offline harness that drives the processor directly (no host, no audio device),
# so the freeze/crossfade/pitch behaviour can be asserted in CI.
add_executable(GranularFreezeTests PluginTests.cpp)

target_link_libraries(GranularFreezeTests PRIVATE
  "$<LINK_ONLY:GranularFreeze>"
  juce::juce_audio_processors)

# Renders demo audio + objective measurements. Not run in CI; a listening aid.
add_executable(GranularFreezeRender RenderDemo.cpp)
target_link_libraries(GranularFreezeRender PRIVATE
  "$<LINK_ONLY:GranularFreeze>" juce::juce_audio_processors juce::juce_audio_formats)
