cmake_minimum_required(VERSION 3.22.1)
project(VAD VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 23)
add_compile_options(-Wall)

set(SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB SOURCES ${SOURCES_DIR}/*.c)
include_directories(${INCLUDE_DIR})

find_package(ALSA REQUIRED)
include_directories(${ALSA_INCLUDE_DIRS})

find_library(MATH_LIBRARY m)

add_executable(vad ${SOURCES})
target_link_libraries(vad ${ALSA_LIBRARIES} ${MATH_LIBRARY})

