cmake_minimum_required(VERSION 3.19)

project(Canvas)

set(CMAKE_CXX_STANDARD 14)

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

file(GLOB canvas_files src/*.cpp)
add_executable(canvas ${canvas_files} ${nanogui_sdl_files})

target_link_libraries(
    canvas
    SDL2
    SDL2_image
    SDL2_ttf
    portaudio_static
    sndfile
    fftw3f
)
