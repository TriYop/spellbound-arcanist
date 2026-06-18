cmake_minimum_required(VERSION 3.22)
project(PM0 VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.13
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

set(CLAP_JUCE_EXTENSIONS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    clap-juce-extensions
    GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(clap-juce-extensions)

# Embed factory presets as binary data
file(GLOB FACTORY_PRESET_FILES "Source/Presets/Factory/*.xml")
juce_add_binary_data(PM0_BinaryData
    HEADER_NAME BinaryData.h
    NAMESPACE   PM0_BinaryData
    SOURCES     ${FACTORY_PRESET_FILES}
)

set(PM0_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND PM0_FORMATS AU)
endif()

juce_add_plugin(PM0
    COMPANY_NAME                "YvanJanet"
    PLUGIN_MANUFACTURER_CODE    Yjnt
    PLUGIN_CODE                 PM0P
    FORMATS                     ${PM0_FORMATS}
    PRODUCT_NAME                "PM0"
    BUNDLE_ID                   "com.yvanjanet.pm0"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    DESCRIPTION                 "Soft pads and drones synthesizer"
)

target_sources(PM0 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/PresetManager.cpp
    Source/Synthesis/Voice.cpp
    Source/Synthesis/Oscillator.cpp
    Source/Synthesis/Filter.cpp
    Source/Synthesis/Envelope.cpp
    Source/Synthesis/LFO.cpp
    Source/UI/PM0LookAndFeel.cpp
)

target_compile_definitions(PM0 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_include_directories(PM0 PRIVATE Source/)

target_compile_features(PM0 PUBLIC cxx_std_20)

target_link_libraries(PM0
    PRIVATE
        PM0_BinaryData
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
        juce::juce_dsp
        clap_juce_extensions
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

clap_juce_extensions_plugin(
    TARGET      PM0
    CLAP_ID     "com.yvanjanet.pm0"
    CLAP_FEATURES synth
    CLAP_PROCESS_EVENTS_RESOLUTION_SAMPLES 64
)

# ── Install rules ─────────────────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(_ARTS "${CMAKE_BINARY_DIR}/PM0_artefacts/${CMAKE_BUILD_TYPE}")

install(DIRECTORY  "${_ARTS}/VST3/PM0.vst3"
        DESTINATION VST3
        COMPONENT   Runtime
        USE_SOURCE_PERMISSIONS)

install(FILES      "${_ARTS}/CLAP/PM0.clap"
        DESTINATION CLAP
        COMPONENT   Runtime
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

install(PROGRAMS   "${_ARTS}/Standalone/PM0"
        DESTINATION bin
        COMPONENT   Runtime)

install(PROGRAMS   "${CMAKE_CURRENT_SOURCE_DIR}/scripts/install.sh"
                   "${CMAKE_CURRENT_SOURCE_DIR}/scripts/uninstall.sh"
        DESTINATION .
        COMPONENT   Runtime)

# ── CPack (TGZ) ───────────────────────────────────────────────────────────────
set(CPACK_GENERATOR                 TGZ)
set(CPACK_PACKAGE_NAME              PM0)
set(CPACK_PACKAGE_VENDOR            YvanJanet)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Soft pads and drones synthesizer")
set(CPACK_PACKAGE_VERSION           ${PROJECT_VERSION})
set(CPACK_SYSTEM_NAME               linux-x86_64)
set(CPACK_PACKAGE_FILE_NAME         "PM0-${PROJECT_VERSION}-linux-x86_64")
set(CPACK_PACKAGING_INSTALL_PREFIX  "")
set(CPACK_STRIP_FILES               TRUE)
set(CPACK_PACKAGE_CHECKSUM          SHA256)
set(CPACK_INSTALL_CMAKE_PROJECTS
    "${CMAKE_BINARY_DIR};${PROJECT_NAME};Runtime;/")
include(CPack)
