# Reactive

Realtime Ray Tracing Engine using Vulkan

## Requirement

- Vulkan SDK
- CMake
- vcpkg

## Build

### Install dependencies

```
vspkg install glm:x64-windows
vspkg install glfw:x64-windows
vspkg install imgui:x64-windows
vspkg install imgui[glfw-binding]:x64-windows
vspkg install imgui[vulkan-binding]:x64-windows
vspkg install imgui[docking-experimental]:x64-windows
vspkg install spdlog:x64-windows
vspkg install tinyobjloader:x64-windows
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
