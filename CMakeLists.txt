cmake_minimum_required(VERSION 3.19)

project(reactive LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

file(TO_CMAKE_PATH $ENV{VULKAN_SDK} VULKAN_SDK)
set(VULKAN_LIB ${VULKAN_SDK}/Lib)
set(VULKAN_INCLUDE ${VULKAN_SDK}/Include)
set(VULKAN_SOURCE ${VULKAN_SDK}/Source)

# -----------------------------------------------
# spirv_reflect
# -----------------------------------------------
file(GLOB_RECURSE spirv_reflect_source ${VULKAN_SOURCE}/SPIRV-Reflect/*)
add_library(spirv_reflect STATIC ${spirv_reflect_source})
set_source_files_properties(
    ${spirv_reflect_source} PROPERTIES LANGUAGE CXX
)

# MSVC でのみ静的解析を無効化
if(MSVC)
  target_compile_options(spirv_reflect PRIVATE /analyze-)
endif()

target_include_directories(spirv_reflect PUBLIC
    ${VULKAN_INCLUDE}/
    ${VULKAN_SOURCE}/
    ${VULKAN_SOURCE}/SPIRV-Reflect/include
)

# -----------------------------------------------
# reactive
# -----------------------------------------------
file(GLOB_RECURSE sources src/*.cpp)
file(GLOB_RECURSE headers include/reactive/*.hpp)
add_library(${PROJECT_NAME} 
    ${sources}
    ${headers}
)

target_precompile_headers(${PROJECT_NAME} PUBLIC include/reactive/pch.h)

find_package(Ktx CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC 
    spirv_reflect
    glfw
    glm::glm
    imgui::imgui
    spdlog::spdlog
    spdlog::spdlog_header_only
    tinyobjloader::tinyobjloader
    KTX::ktx

    optimized ${VULKAN_LIB}/spirv-cross-core.lib
    optimized ${VULKAN_LIB}/spirv-cross-cpp.lib
    optimized ${VULKAN_LIB}/spirv-cross-glsl.lib
    optimized ${VULKAN_LIB}/spirv-cross-reflect.lib

    optimized ${VULKAN_LIB}/OSDependent.lib
    optimized ${VULKAN_LIB}/SPIRV-Tools.lib
    optimized ${VULKAN_LIB}/SPIRV-Tools-opt.lib

    optimized ${VULKAN_LIB}/glslang-default-resource-limits.lib
    optimized ${VULKAN_LIB}/glslang.lib
    optimized ${VULKAN_LIB}/GenericCodeGen.lib
    optimized ${VULKAN_LIB}/MachineIndependent.lib
    optimized ${VULKAN_LIB}/SPVRemapper.lib
    optimized ${VULKAN_LIB}/SPIRV.lib

    optimized ${VULKAN_LIB}/slang.lib
    
    debug ${VULKAN_LIB}/spirv-cross-cored.lib
    debug ${VULKAN_LIB}/spirv-cross-cppd.lib
    debug ${VULKAN_LIB}/spirv-cross-glsld.lib
    debug ${VULKAN_LIB}/spirv-cross-reflectd.lib
    
    debug ${VULKAN_LIB}/OSDependentd.lib
    debug ${VULKAN_LIB}/SPIRV-Toolsd.lib
    debug ${VULKAN_LIB}/SPIRV-Tools-optd.lib

    debug ${VULKAN_LIB}/glslang-default-resource-limitsd.lib
    debug ${VULKAN_LIB}/glslangd.lib
    debug ${VULKAN_LIB}/GenericCodeGend.lib
    debug ${VULKAN_LIB}/MachineIndependentd.lib
    debug ${VULKAN_LIB}/SPVRemapperd.lib
    debug ${VULKAN_LIB}/SPIRVd.lib

    debug ${VULKAN_LIB}/slangd.lib
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    "ASSET_DIR=std::string{\"${PROJECT_SOURCE_DIR}/asset/\"}"
)

# -----------------------------------------------
# sample
# -----------------------------------------------
option(REACTIVE_BUILD_SAMPLES "Build the Reactive samples" ON)
if (REACTIVE_BUILD_SAMPLES)
    add_subdirectory(sample/hello_graphics)
    add_subdirectory(sample/hello_raytracing)
    add_subdirectory(sample/hello_compute)
    add_subdirectory(sample/hello_mesh_shader)
endif()
