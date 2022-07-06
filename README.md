# Reactive

Realtime Ray Tracing Engine using Vulkan

## Requirement

- Vulkan SDK
- CMake
- vcpkg

## Build

### Install dependencies

```
vcpkg install glm:x64-windows
vcpkg install glfw:x64-windows
vcpkg install imgui:x64-windows
vcpkg install imgui[glfw-binding]:x64-windows
vcpkg install imgui[vulkan-binding]:x64-windows
vcpkg install imgui[docking-experimental]:x64-windows
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
