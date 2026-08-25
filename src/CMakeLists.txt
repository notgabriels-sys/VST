add_library(GranularFreezeCore STATIC
  dsp/GrainEngine.cpp
  dsp/GranularFreezeCore.cpp
  dsp/SpectralAnalyzer.cpp)
target_include_directories(GranularFreezeCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(GranularFreezeCore PUBLIC cxx_std_17)
dpf_add_plugin(GranularFreeze TARGETS vst3 clap au
  FILES_DSP GranularFreezePlugin.cpp
  FILES_UI GranularFreezeUI.cpp)
target_include_directories(GranularFreeze PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(GranularFreeze PUBLIC GranularFreezeCore)

if(APPLE)
  # DPF's AU exporter currently emits a broad resourceUsage dictionary that
  # claims network and arbitrary file read/write access. Granular Freeze is an
  # offline effect: it performs no network or file I/O, so advertising those
  # resources would unnecessarily make the AU non-sandbox-safe to hosts.
  # Remove the declaration after DPF generates the plist and fail closed if
  # the generated metadata changes or plutil cannot validate the result.
  add_custom_command(TARGET GranularFreeze-au POST_BUILD
    COMMAND /usr/bin/plutil -remove AudioComponents.0.resourceUsage
      "$<TARGET_FILE_DIR:GranularFreeze-au>/../Info.plist"
    COMMAND /usr/bin/plutil -lint
      "$<TARGET_FILE_DIR:GranularFreeze-au>/../Info.plist"
    COMMENT "Removing unnecessary AU resource declarations")
endif()
