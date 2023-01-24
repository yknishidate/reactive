#pragma once
#include "Mesh.hpp"

class Image;

namespace Loader {
void loadFromFile(const std::string& filepath,
                  std::vector<Vertex>& vertices,
                  std::vector<uint32_t>& indices);

void loadFromFile(const std::string& filepath,
                  std::vector<Mesh>& outMeshes,
                  std::vector<Material>& outMaterials);

void loadFromFile(const std::string& filepath,
                  std::vector<Mesh>& outMeshes,
                  std::vector<Material>& outMaterials,
                  std::vector<Image>& outTextures);

void loadFromFile(const std::string& filepath,
                  std::vector<Vertex>& outVertices,
                  std::vector<std::vector<uint32_t>>& outIndices,
                  std::vector<Material>& outMaterials,
                  std::vector<Image>& outTextures);
}  // namespace Loader
