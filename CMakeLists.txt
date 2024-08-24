cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

project(tangram VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(TANGRAM_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h")
file(GLOB_RECURSE SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")
file(GLOB_RECURSE SOURCE_FILES_CC "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cc")
file(GLOB_RECURSE SOURCE_FILES_C "${CMAKE_CURRENT_SOURCE_DIR}/source/*.c")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)

#zstd lib

# boost graph



#glslang lib
link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/glslang/lib/glslangd.lib)
link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/glslang/lib/MachineIndependentd.lib)
link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/glslang/lib/GenericCodeGend.lib)
link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/glslang/lib/glslang-default-resource-limitsd.lib)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/glslang)

#
add_definitions(-D TANGRAM_DIR=${TANGRAM_ROOT_DIR})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/Source" FILES ${HEADER_FILES} ${SOURCE_FILES} ${SOURCE_FILES_CC} ${SOURCE_FILES_C})
add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES} ${SOURCE_FILES_CC} ${SOURCE_FILES_C})