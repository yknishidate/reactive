cmake_minimum_required(VERSION 3.16)

set(TARGET_NAME "Slang")

file(GLOB_RECURSE sources *.cpp)
file(GLOB_RECURSE headers *.hpp)
file(GLOB_RECURSE shaders shader/*)
add_executable(${TARGET_NAME} ${sources} ${headers} ${shaders})

source_group("Shader Files" FILES ${shaders})

target_link_libraries(${TARGET_NAME} PRIVATE 
    reactive
)

target_include_directories(${TARGET_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_definitions(${TARGET_NAME} PRIVATE
    "ASSET_DIR=std::string{\"${PROJECT_SOURCE_DIR}/asset/\"}")
target_compile_definitions(${TARGET_NAME} PRIVATE
    "SHADER_DIR=std::string{\"${CMAKE_CURRENT_SOURCE_DIR}/shader/\"}")
