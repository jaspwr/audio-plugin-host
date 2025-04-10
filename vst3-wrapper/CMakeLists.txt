cmake_minimum_required(VERSION 3.8)

project(vst3wrapper)

include(cmake/get_cpm.cmake)
include(cmake/Dependencies.cmake)

set(vst3wrapper_sources
    # ${SDK_ROOT}/public.sdk/source/vst/hosting/plugprovider.cpp
    # ${SDK_ROOT}/public.sdk/source/vst/hosting/plugprovider.h
    source/vst3wrapper.cpp
    source/vst3wrapper.h
    # source/memoryibstream.h
)

set(target vst3wrapper)
add_library(${target} STATIC ${vst3wrapper_sources})
target_compile_features(${target}
    PUBLIC
        cxx_std_17
)

target_compile_features(vst3wrapper
    PUBLIC
        cxx_std_17
)

target_compile_definitions(vst3wrapper
    PUBLIC
        "-DSDL_MAIN_HANDLED"
)

target_link_libraries(vst3wrapper
    PRIVATE
        VST_SDK
        SDL2-static
        portaudio
        RtMidi
        concurrentqueue
)

target_include_directories(vst3wrapper
    PRIVATE
        include
)


