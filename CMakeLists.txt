cmake_minimum_required(VERSION 3.19)

project(reactive LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE sources src/*.cpp)
file(GLOB_RECURSE headers include/reactive/*.hpp)
add_library(${PROJECT_NAME} ${sources} ${headers})

target_precompile_headers(${PROJECT_NAME} PUBLIC include/reactive/pch.h)

find_package(Ktx CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)

set(VULKAN_LIB $ENV{VULKAN_SDK}/Lib)
set(VULKAN_INCLUDE $ENV{VULKAN_SDK}/Include)

target_link_libraries(${PROJECT_NAME} PUBLIC 
    glfw
    glm::glm
    imgui::imgui
    spdlog::spdlog
    spdlog::spdlog_header_only
    tinyobjloader::tinyobjloader
    KTX::ktx
    glslang::glslang glslang::glslang-default-resource-limits glslang::SPIRV glslang::SPVRemapper
    spirv-cross-core
    spirv-cross-glsl
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${VULKAN_INCLUDE}/
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    "ASSET_DIR=std::string{\"${PROJECT_SOURCE_DIR}/asset/\"}")

option(REACTIVE_BUILD_SAMPLES "Build the Reactive samples" ON)
if (REACTIVE_BUILD_SAMPLES)
    add_subdirectory(sample/hello_graphics)
    add_subdirectory(sample/hello_raytracing)
    add_subdirectory(sample/hello_compute)
    add_subdirectory(sample/hello_mesh_shader)
endif()
