# Reactive

Personal Vulkan wrapper using

- Vulkan
- GLFW
- glm
- ImGui
- glslang
- SPIRV-Cross
- tinyobjloader
- spdlog

## Sample

![image](https://user-images.githubusercontent.com/30839669/236371719-bb247384-52b9-4096-8739-d1ebe6bb1620.png)

## Features

- [x] Vulkan wrapper
- [x] Window creation
- [x] ImGui integration
- [x] GLSL Shader compile
- [x] Shader reflection

## Requirement

- Vulkan SDK
- CMake

## Run cmake

```sh
# For Windows
cmake . --preset vs
```

## Usage (in your project)

1. Create project

```sh
mkdir project_name
cd project_name
git init
git submodule add https://github.com/yknishidate/reactive.git
```

2. Add `main.cpp`

```
project_name/
 - reactive/
 - main.cpp
```

```cpp
class HelloApp : public App {
public:
    HelloApp()
        : App({
              .width = 1280,
              .height = 720,
              .title = "HelloGraphics",
          }) {}
};

int main() {
    try {
        HelloApp app{};
        app.run();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
    }
}
```

3. Add `CMakeLists.txt`

```
project_name/
  - reactive/
  - main.cpp
  - CMakeLists.txt
```

```cmake
cmake_minimum_required(VERSION 3.16)

project(project_name LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

set(REACTIVE_BUILD_SAMPLES OFF CACHE BOOL "" FORCE) # Remove samples
add_subdirectory(reactive) # Add Reactive

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC 
    reactive
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}
    reactive/include
)
```

4. Run cmake

```sh
# change to your vcpkg path
cmake . -B build -D CMAKE_TOOLCHAIN_FILE=.\reactive\vcpkg\scripts\buildsystems\vcpkg.cmake
```
