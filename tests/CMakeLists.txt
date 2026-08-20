# Offline harness that drives the processor directly (no host, no audio device),
# so the freeze/crossfade/pitch behaviour can be asserted in CI.
juce_add_console_app(GranularFreezeTests PRODUCT_NAME "GranularFreezeTests")

target_sources(GranularFreezeTests PRIVATE PluginTests.cpp)

juce_generate_juce_header(GranularFreezeTests)

target_link_libraries(GranularFreezeTests PRIVATE
  GranularFreeze
  juce::juce_audio_processors
  juce::juce_audio_utils)

juce_add_console_app(GranularFreezeEngineTests PRODUCT_NAME "GranularFreezeEngineTests")
target_sources(GranularFreezeEngineTests PRIVATE GrainEngineTests.cpp)
juce_generate_juce_header(GranularFreezeEngineTests)
target_link_libraries(GranularFreezeEngineTests PRIVATE
  GranularFreeze
  juce::juce_audio_processors
  juce::juce_audio_utils)

# Renders demo audio + objective measurements. Not run in CI; a listening aid.
juce_add_console_app(GranularFreezeRender PRODUCT_NAME "GranularFreezeRender")
target_sources(GranularFreezeRender PRIVATE RenderDemo.cpp)
juce_generate_juce_header(GranularFreezeRender)
target_link_libraries(GranularFreezeRender PRIVATE
  GranularFreeze juce::juce_audio_processors juce::juce_audio_utils juce::juce_audio_formats)
