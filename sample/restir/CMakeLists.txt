cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE sources *.cpp)
file(GLOB_RECURSE headers *.hpp)
file(GLOB_RECURSE shaders shader/*)
add_executable(ReSTIR ${sources} ${headers} ${shaders})

source_group("Shader Files" FILES ${shaders})

target_link_libraries(ReSTIR PRIVATE 
    Reactive
)

target_include_directories(ReSTIR PUBLIC
    ${PROJECT_SOURCE_DIR}/source
    ${PROJECT_SOURCE_DIR}/
)

target_compile_definitions(ReSTIR PRIVATE
    "ASSET_DIR=std::string{\"${PROJECT_SOURCE_DIR}/asset/\"}")
target_compile_definitions(ReSTIR PRIVATE
    "SHADER_DIR=std::string{\"${CMAKE_CURRENT_SOURCE_DIR}/shader/\"}")
