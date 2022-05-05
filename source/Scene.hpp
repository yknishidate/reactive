#pragma once
#include <vector>
#include <memory>
#include "Mesh.hpp"

class Scene
{
public:
    explicit Scene(const std::string& filepath);

private:
    std::vector<std::shared_ptr<Mesh>> meshes;
};
