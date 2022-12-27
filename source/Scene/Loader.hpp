#pragma once
#include "Mesh.hpp"

class Image;

namespace Loader
{
void loadFromFile(const std::string& filepath,
                  std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

void loadFromFile(const std::string& filepath,
                  std::vector<std::shared_ptr<Mesh>>& meshes);

void loadFromFile(const std::string& filepath,
                  std::vector<std::shared_ptr<Mesh>>& meshes,
                  std::vector<Image>& textures);
}
