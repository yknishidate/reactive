# Reactive

Personal Vulkan Wrapper

## Requirement

- Vulkan SDK
- CMake
- vcpkg

## Run cmake

```sh
mkdir build
cd build

# change to your vcpkg path
cmake .. -D CMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
```

## Usage (in your project)

1. Create project

```sh
mkdir project_name
cd project_name
git init
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
        Context::init(true);
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
cmake . -B build -D CMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
```
