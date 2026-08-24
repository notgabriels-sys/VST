add_library(GranularFreezeCore STATIC dsp/GrainEngine.cpp dsp/GranularFreezeCore.cpp)
target_include_directories(GranularFreezeCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(GranularFreezeCore PUBLIC cxx_std_17)
dpf_add_plugin(GranularFreeze TARGETS vst3 clap au
  FILES_DSP GranularFreezePlugin.cpp
  FILES_UI GranularFreezeUI.cpp)
target_include_directories(GranularFreeze PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(GranularFreeze PUBLIC GranularFreezeCore)
