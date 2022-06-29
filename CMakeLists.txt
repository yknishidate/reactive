cmake_minimum_required(VERSION 3.16)

include($ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)

project(Reactive LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

option(ENABLE_HLSL OFF)

add_subdirectory(external/glslang)

file(GLOB_RECURSE sources source/*.cpp)
file(GLOB_RECURSE headers source/*.hpp)
add_executable(${PROJECT_NAME} 
    ${sources}
    ${headers}
    external/imgui-vulkan-hpp/imgui_impl_vulkan_hpp.cpp
)

target_precompile_headers(${PROJECT_NAME} PUBLIC source/pch.h)

find_package(glfw3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
# find_package(glslang CONFIG REQUIRED)

target_link_libraries(
    ${PROJECT_NAME} PUBLIC 
    $ENV{VULKAN_SDK}/Lib/vulkan-1.lib
    
    optimized $ENV{VULKAN_SDK}/Lib/spirv-cross-c.lib
    optimized $ENV{VULKAN_SDK}/Lib/spirv-cross-c-shared.lib
    optimized $ENV{VULKAN_SDK}/Lib/spirv-cross-core.lib
    optimized $ENV{VULKAN_SDK}/Lib/spirv-cross-cpp.lib
    optimized $ENV{VULKAN_SDK}/Lib/spirv-cross-glsl.lib
    
    debug $ENV{VULKAN_SDK}/Lib/spirv-cross-cd.lib
    debug $ENV{VULKAN_SDK}/Lib/spirv-cross-c-sharedd.lib
    debug $ENV{VULKAN_SDK}/Lib/spirv-cross-cored.lib
    debug $ENV{VULKAN_SDK}/Lib/spirv-cross-cppd.lib
    debug $ENV{VULKAN_SDK}/Lib/spirv-cross-glsld.lib

    spdlog::spdlog spdlog::spdlog_header_only
    glm::glm
    glfw
    glslang
    glslang-default-resource-limits
    SPIRV
    tinyobjloader::tinyobjloader
    imgui::imgui
    # HLSL SPIRV glslang OGLCompiler
)
set_property(TARGET glfw PROPERTY FOLDER "External")
set_property(TARGET glslang PROPERTY FOLDER "External")
set_property(TARGET glslang-default-resource-limits PROPERTY FOLDER "External")

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    $ENV{VULKAN_SDK}/Include/spirv_cross
    "${PROJECT_SOURCE_DIR}/external/vulkan/include"
    "${PROJECT_SOURCE_DIR}/external/glslang"
    "${PROJECT_SOURCE_DIR}/external/imgui-vulkan-hpp"
    "${PROJECT_SOURCE_DIR}/source"
    "${PROJECT_SOURCE_DIR}/"
)
