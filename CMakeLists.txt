cmake_minimum_required(VERSION 3.19)

project(Canvas)

option(CANVAS_BENCHMARK "Create benchmark executable" OFF)

set(CMAKE_CXX_STANDARD 14)

set(
    CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules
    ${CMAKE_MODULE_PATH}
)

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SndFile REQUIRED)
find_package(FFTW REQUIRED)

# Aliases for NanoGUI-SDL's sake.
set(SDL2TTF_LIBRARY ${SDL2_TTF_LIBRARY})
set(SDL2TTF_INCLUDE_DIR ${SDL2_TTF_INCLUDE_DIR})

add_subdirectory(third_party/portaudio)
include_directories(
    third_party/portaudio/include/
    third_party/portaudio/src/common/
)
if(APPLE)
    include_directories(/usr/local/include)
    link_directories(/usr/local/lib)
endif()

set(NANOGUI_BUILD_EXAMPLE OFF)
add_subdirectory(third_party/nanogui-sdl)
include_directories(
    third_party/nanogui-sdl
)
file(GLOB nanogui_sdl_files
    third_party/nanogui-sdl/sdlgui/*.cpp
    third_party/nanogui-sdl/sdlgui/*.c
)

include_directories(third_party/stb)

include_directories(third_party/tclap/include)

file(GLOB canvas_files src/*.cpp)
add_executable(canvas ${canvas_files} ${nanogui_sdl_files})

if(UNIX AND NOT APPLE)
    target_link_libraries(
        canvas
        SDL2
        SDL2_image
        SDL2_ttf
        portaudio_static
        sndfile
        fftw3f
    )
else()
    target_include_directories(
        canvas
        PRIVATE
        ${SDL2_INCLUDE_DIR}
        ${SDL2_IMAGE_INCLUDE_DIR}
        ${SDL2_TTF_INCLUDE_DIR}
    )
    target_link_libraries(
        canvas
        PRIVATE
        ${SDL2_LIBRARY}
        ${SDL2_IMAGE_LIBRARY}
        ${SDL2_TTF_LIBRARY}
    )

    target_include_directories(canvas PRIVATE ${SNDFILE_INCLUDE_DIR})
    target_link_libraries(canvas PRIVATE ${SNDFILE_LIBRARY})

    target_include_directories(canvas PRIVATE ${FFTW_INCLUDE_DIRS})
    target_link_libraries(canvas PRIVATE ${FFTW_LIBRARIES})

    target_link_libraries(canvas PRIVATE portaudio_static)
endif()

if(CANVAS_BENCHMARK)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
    add_subdirectory(third_party/benchmark)
    add_executable(
        canvas_benchmark
        src/benchmark/benchmark.cpp
        src/Synth.hpp
        src/Synth.cpp
    )
    target_link_libraries(canvas_benchmark benchmark::benchmark portaudio_static)
endif()
