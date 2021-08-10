cmake_minimum_required(VERSION 3.19)

project(Canvas)

add_subdirectory(third_party/portaudio)
include_directories(
    third_party/portaudio/include/
    third_party/portaudio/src/common/
)
SET(CMAKE_CXX_STANDARD 11)

if(APPLE)
    include_directories(/usr/local/include)
    link_directories(/usr/local/lib)
endif()

file(GLOB canvas_files src/*.cpp)
add_executable(canvas ${canvas_files})

target_link_libraries(
    canvas
    SDL2
    portaudio_static
)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -Wpedantic")
endif()
