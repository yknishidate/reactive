#pragma once
#include <tiny_obj_loader.h>
#include "Mesh.hpp"

class Image;

namespace Loader
{
    void LoadFromFile(const std::string& filepath,
                      std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

    void LoadFromFile(const std::string& filepath,
                      std::vector<std::shared_ptr<Mesh>>& meshes);

    void LoadFromFile(const std::string& filepath,
                      std::vector<std::shared_ptr<Mesh>>& meshes,
                      std::vector<Image>& textures);
}
