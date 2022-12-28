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
