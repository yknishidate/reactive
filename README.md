# Reactive

Realtime Ray Tracing Engine using Vulkan

## Requirement

- Vulkan SDK
- CMake
- vcpkg

## Build

### Install dependencies

```
vcpkg install stb:x64-windows
vcpkg install glm:x64-windows
vcpkg install glfw3:x64-windows
vcpkg install imgui[docking-experimental,glfw-binding,vulkan-binding]:x64-windows
vcpkg install spdlog:x64-windows
vcpkg install tinyobjloader:x64-windows
```

### Set env variable

```
VCPKG_ROOT=/path/to/your/vcpkg
```

### Run

```
mkdir build
cd build
cmake ..

cmake --build .

Debug\Reactive.exe
```

## Samples

- [Hello Graphics](sample/hello_graphics/)
- [Hello Raytracing](sample/hello_raytracing/)
- [Light Field](sample/light_field/)
- [ReSTIR](sample/restir/)

## Usage (in your project)

1. Create project

```sh
# make your project
mkdir project_name
cd project_name

# init git
git init

# add Reactive
git submodule add https://github.com/yknishidate/Reactive.git
```

2. Add `main.cpp`

```
project_name/
  - Reactive/
  - main.cpp
```

```cpp
#include "Engine.hpp"

int main()
{
    try {
        Log::init();
        Window::init(750, 750);
        while (!Window::shouldClose()) {
            Window::pollEvents();
        }
        Context::waitIdle();
        Window::shutdown();
    } catch (const std::exception& e) {
        Log::error(e.what());
    }
}
```

3. Add `CMakeLists.txt`

```
project_name/
  - Reactive/
  - main.cpp
  - CMakeLists.txt
```

```cmake
cmake_minimum_required(VERSION 3.16)

# Use vcpkg
include($ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)

project(project_name LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

set(REACTIVE_BUILD_SAMPLES OFF CACHE BOOL "" FORCE) # Remove samples
add_subdirectory(Reactive) # Add Reactive

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC 
    Reactive
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}
    Reactive/source
)
```

4. Run cmake

```sh
cmake . -B build
```
