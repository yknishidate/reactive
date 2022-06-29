cmake_minimum_required(VERSION 3.16)

include($ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)

project(Reactive LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

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

target_link_libraries(
    ${PROJECT_NAME} PUBLIC 
    $ENV{VULKAN_SDK}/Lib/vulkan-1.lib
    
    optimized $ENV{VULKAN_SDK}/Lib/spirv-cross-core.lib
    optimized $ENV{VULKAN_SDK}/Lib/spirv-cross-cpp.lib
    optimized $ENV{VULKAN_SDK}/Lib/spirv-cross-glsl.lib

    optimized $ENV{VULKAN_SDK}/Lib/OGLCompiler.lib
    optimized $ENV{VULKAN_SDK}/Lib/OSDependent.lib
    optimized $ENV{VULKAN_SDK}/Lib/SPIRV-Tools.lib
    optimized $ENV{VULKAN_SDK}/Lib/SPIRV-Tools-opt.lib

    optimized $ENV{VULKAN_SDK}/Lib/glslang-default-resource-limits.lib
    optimized $ENV{VULKAN_SDK}/Lib/glslang.lib
    optimized $ENV{VULKAN_SDK}/Lib/GenericCodeGen.lib
    optimized $ENV{VULKAN_SDK}/Lib/MachineIndependent.lib
    optimized $ENV{VULKAN_SDK}/Lib/SPVRemapper.lib
    optimized $ENV{VULKAN_SDK}/Lib/SPIRV.lib
    
    debug $ENV{VULKAN_SDK}/Lib/spirv-cross-cored.lib
    debug $ENV{VULKAN_SDK}/Lib/spirv-cross-cppd.lib
    debug $ENV{VULKAN_SDK}/Lib/spirv-cross-glsld.lib
    
    debug $ENV{VULKAN_SDK}/Lib/OGLCompilerd.lib
    debug $ENV{VULKAN_SDK}/Lib/OSDependentd.lib
    debug $ENV{VULKAN_SDK}/Lib/SPIRV-Toolsd.lib
    debug $ENV{VULKAN_SDK}/Lib/SPIRV-Tools-optd.lib

    debug $ENV{VULKAN_SDK}/Lib/glslang-default-resource-limitsd.lib
    debug $ENV{VULKAN_SDK}/Lib/glslangd.lib
    debug $ENV{VULKAN_SDK}/Lib/GenericCodeGend.lib
    debug $ENV{VULKAN_SDK}/Lib/MachineIndependentd.lib
    debug $ENV{VULKAN_SDK}/Lib/SPVRemapperd.lib
    debug $ENV{VULKAN_SDK}/Lib/SPIRVd.lib

    spdlog::spdlog spdlog::spdlog_header_only
    glm::glm
    glfw
    tinyobjloader::tinyobjloader
    imgui::imgui
)

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    $ENV{VULKAN_SDK}/Include/spirv_cross
    $ENV{VULKAN_SDK}/Include/glslang
    $ENV{VULKAN_SDK}/Include/glslang/SPIRV
    $ENV{VULKAN_SDK}/Include/glslang/Include
    $ENV{VULKAN_SDK}/Include/glslang/Public
    "${PROJECT_SOURCE_DIR}/external/vulkan/include"
    "${PROJECT_SOURCE_DIR}/external/imgui-vulkan-hpp"
    "${PROJECT_SOURCE_DIR}/source"
    "${PROJECT_SOURCE_DIR}/"
)
