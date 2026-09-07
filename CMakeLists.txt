cmake_minimum_required(VERSION 3.22)
project(Arcanist VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# DPF has no tagged releases -- pin the exact commit SHA Hex/Pugilist/Tank/
# Outflank all use.
set(AUDIOPLUGINS_DPF_GIT_TAG "4238e1c7f0351bbe488d79f0899c540543ac7583" CACHE STRING
    "Pinned DPF commit (DISTRHO/DPF has no git tags to pin to instead)")

# CAVEAT (CMake FetchContent): the patch step only runs on a *fresh*
# population of the source dir -- delete build/_deps/dpf-* to force a
# clean re-fetch if you need to be sure the patch actually took effect.
FetchContent_Declare(dpf
    GIT_REPOSITORY https://github.com/DISTRHO/DPF.git
    GIT_TAG        ${AUDIOPLUGINS_DPF_GIT_TAG}
    GIT_SHALLOW    TRUE
    PATCH_COMMAND  ${CMAKE_COMMAND}
                   -DPATCH_FILE=${CMAKE_CURRENT_SOURCE_DIR}/cmake/patches/dpf-clap-state-chunked-read.patch
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/apply_patch.cmake
)
FetchContent_MakeAvailable(dpf)

set(Arcanist_DPF_TARGETS vst3 clap lv2 jack)
if(APPLE)
    list(APPEND Arcanist_DPF_TARGETS au)
endif()

dpf_add_plugin(Arcanist
    TARGETS ${Arcanist_DPF_TARGETS}
    UI_TYPE opengl
    # ArcanistUI's "SAVE AS" button (Task 6) opens a native save dialog via
    # UI::openFileBrowser() -- DPF requires this CMake option (which defines
    # DGL_USE_FILE_BROWSER) whenever DISTRHO_UI_FILE_BROWSER is set in
    # DistrhoPluginInfo.h, or DistrhoPluginChecks.h fails the build outright.
    USE_FILE_BROWSER
    FILES_DSP
        Source/ArcanistPluginAdapter.cpp
        Source/Synthesis/Voice.cpp
        Source/Synthesis/Oscillator.cpp
        Source/Synthesis/Filter.cpp
        Source/Synthesis/Envelope.cpp
        Source/Synthesis/LFO.cpp
    FILES_UI
        Source/ArcanistUI.cpp
)

target_include_directories(Arcanist PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Source")
target_compile_features(Arcanist PUBLIC cxx_std_20)

# Same DPF WebViewImpl.cpp / GNU `typeof` workaround Hex/Pugilist/Tank need.
if(TARGET dgl-opengl)
    set_target_properties(dgl-opengl PROPERTIES CXX_EXTENSIONS ON)
endif()

# ── Shared HUI/presets library ───────────────────────────────────────────────
# ORDERING (load-bearing): must come after dpf_add_plugin(Arcanist ...) --
# DPF creates its DGL target lazily inside that call.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

FetchContent_Declare(AudioPluginsCommon
    GIT_REPOSITORY https://github.com/TriYop/spellbound-common.git
    GIT_TAG        v0.4.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(AudioPluginsCommon)

target_link_libraries(Arcanist-ui PRIVATE AudioPluginsCommon::hui_dgl AudioPluginsCommon::presets)

# ── Unit tests (no JUCE/DPF dependency) ──────────────────────────────────────
enable_testing()

add_executable(test_audiobuffer Tests/test_audiobuffer.cpp)
target_include_directories(test_audiobuffer PRIVATE Source/ Tests/)
target_compile_features(test_audiobuffer PRIVATE cxx_std_20)
add_test(NAME AudioBuffer COMMAND test_audiobuffer)

add_executable(test_dsputil Tests/test_dsputil.cpp)
target_include_directories(test_dsputil PRIVATE Source/ Tests/)
target_compile_features(test_dsputil PRIVATE cxx_std_20)
add_test(NAME DspUtil COMMAND test_dsputil)

add_executable(test_oscillator Tests/test_oscillator.cpp Source/Synthesis/Oscillator.cpp)
target_include_directories(test_oscillator PRIVATE Source/ Tests/)
target_compile_features(test_oscillator PRIVATE cxx_std_20)
add_test(NAME Oscillator COMMAND test_oscillator)

add_executable(test_filter Tests/test_filter.cpp Source/Synthesis/Filter.cpp)
target_include_directories(test_filter PRIVATE Source/ Tests/)
target_compile_features(test_filter PRIVATE cxx_std_20)
add_test(NAME Filter COMMAND test_filter)

add_executable(test_envelope Tests/test_envelope.cpp Source/Synthesis/Envelope.cpp)
target_include_directories(test_envelope PRIVATE Source/ Tests/)
target_compile_features(test_envelope PRIVATE cxx_std_20)
add_test(NAME Envelope COMMAND test_envelope)

add_executable(test_lfo Tests/test_lfo.cpp Source/Synthesis/LFO.cpp)
target_include_directories(test_lfo PRIVATE Source/ Tests/)
target_compile_features(test_lfo PRIVATE cxx_std_20)
add_test(NAME LFO COMMAND test_lfo)

add_executable(test_voice
    Tests/test_voice.cpp
    Source/Synthesis/Voice.cpp Source/Synthesis/Oscillator.cpp
    Source/Synthesis/Filter.cpp Source/Synthesis/Envelope.cpp Source/Synthesis/LFO.cpp
)
target_include_directories(test_voice PRIVATE Source/ Tests/)
target_compile_features(test_voice PRIVATE cxx_std_20)
add_test(NAME Voice COMMAND test_voice)

add_executable(test_factorypresets Tests/test_factorypresets.cpp)
target_include_directories(test_factorypresets PRIVATE Source/ Tests/)
target_link_libraries(test_factorypresets PRIVATE AudioPluginsCommon::presets)
target_compile_features(test_factorypresets PRIVATE cxx_std_20)
add_test(NAME FactoryPresets COMMAND test_factorypresets)

# Task 6 (RadioButtonGroup, if it gains its own test) appends its own block
# here once that file exists -- deliberately not included yet.

# ── Install rules ─────────────────────────────────────────────────────────────
set(_BIN "${CMAKE_BINARY_DIR}/bin")

install(DIRECTORY  "${_BIN}/Arcanist.vst3"
        DESTINATION VST3
        COMPONENT   Runtime
        USE_SOURCE_PERMISSIONS)

if(APPLE)
    install(DIRECTORY  "${_BIN}/Arcanist.clap"
            DESTINATION CLAP
            COMPONENT   Runtime
            USE_SOURCE_PERMISSIONS)
else()
    install(FILES      "${_BIN}/Arcanist.clap"
            DESTINATION CLAP
            COMPONENT   Runtime
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                        GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()

install(DIRECTORY  "${_BIN}/Arcanist.lv2"
        DESTINATION LV2
        COMPONENT   Runtime
        USE_SOURCE_PERMISSIONS)

install(PROGRAMS   "${_BIN}/Arcanist"
        DESTINATION bin
        COMPONENT   Runtime)

install(PROGRAMS   "${CMAKE_CURRENT_SOURCE_DIR}/scripts/install.sh"
                   "${CMAKE_CURRENT_SOURCE_DIR}/scripts/uninstall.sh"
        DESTINATION .
        COMPONENT   Runtime)

# ── CPack (TGZ) ───────────────────────────────────────────────────────────────
set(CPACK_GENERATOR                 TGZ)
set(CPACK_PACKAGE_NAME              Arcanist)
set(CPACK_PACKAGE_VENDOR            Spellbound)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Soft pads and drones synthesizer")
set(CPACK_PACKAGE_VERSION           ${PROJECT_VERSION})
set(CPACK_SYSTEM_NAME               linux-x86_64)
set(CPACK_PACKAGE_FILE_NAME         "Arcanist-${PROJECT_VERSION}-linux-x86_64")
set(CPACK_PACKAGING_INSTALL_PREFIX  "")
set(CPACK_STRIP_FILES               TRUE)
set(CPACK_PACKAGE_CHECKSUM          SHA256)
set(CPACK_INSTALL_CMAKE_PROJECTS
    "${CMAKE_BINARY_DIR};${PROJECT_NAME};Runtime;/")
include(CPack)
