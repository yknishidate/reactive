// #include "Loader.hpp"
// #include <spdlog/spdlog.h>
// #include <algorithm>
// #include <filesystem>
// #include <set>
// #include <string>
// #include <unordered_map>
// #include "Graphics/Image.hpp"
//
// #define TINYOBJLOADER_IMPLEMENTATION
// #include <tiny_obj_loader.h>
//
// void loadShape(const tinyobj::attrib_t& attrib,
//                const tinyobj::shape_t& shape,
//                std::vector<Vertex>& vertices,
//                std::vector<uint32_t>& indices) {
//     for (const auto& index : shape.mesh.indices) {
//         Vertex vertex{};
//
//         vertex.pos = {
//             attrib.vertices[3 * index.vertex_index + 0],
//             -attrib.vertices[3 * index.vertex_index + 1],
//             attrib.vertices[3 * index.vertex_index + 2],
//         };
//
//         if (attrib.normals.empty()) {
//             throw std::runtime_error("This shape doesn't have normals.");
//         }
//         vertex.normal = {
//             attrib.normals[3 * index.normal_index + 0],
//             -attrib.normals[3 * index.normal_index + 1],
//             attrib.normals[3 * index.normal_index + 2],
//         };
//
//         if (attrib.texcoords.empty()) {
//             throw std::runtime_error("This shape doesn't have texcoords.");
//         }
//         if (index.texcoord_index != -1) {  // this mesh doesn't have texcoords
//             vertex.texCoord = {
//                 attrib.texcoords[2 * index.texcoord_index + 0],
//                 1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
//             };
//         }
//
//         vertices.push_back(vertex);
//         indices.push_back(indices.size());
//     }
// }
//
// void loadShapes(const tinyobj::attrib_t& attrib,
//                 const std::vector<tinyobj::shape_t>& shapes,
//                 const std::vector<Material>& mats,
//                 std::vector<Mesh>& meshes) {
//     meshes.reserve(shapes.size());
//     for (const auto& shape : shapes) {
//         assert(!shapes.empty());
//         assert(!mats.empty());
//         spdlog::info("  Shape {}", shape.name);
//
//         std::set<int> unique_material_ids;
//         for (auto& i : shape.mesh.material_ids) {
//             unique_material_ids.insert(i);
//         }
//         spdlog::info("  Materials {}", fmt::join(unique_material_ids, ", "));
//
//         std::vector<Vertex> vertices{};
//         std::vector<uint32_t> indices{};
//         loadShape(attrib, shape, vertices, indices);
//         meshes.emplace_back(vertices, indices, shape.mesh.material_ids[0]);
//     }
// }
//
// void Loader::loadTextures(const std::string& directory,
//                           int texCount,
//                           const std::unordered_map<std::string, int>& textureNames,
//                           std::vector<Image>& textures) {
//     textures.resize(texCount);
//     for (auto& [name, index] : textureNames) {
//         std::string path = name;
//         std::ranges::replace(path, '\\', '/');
//         path = directory + "/" + path;
//         spdlog::info("  Texture {}: {}", index, path);
//         textures[index] = Image{path};
//     }
// }
//
// void Loader::loadFromFile(const std::string& filepath,
//                           std::vector<Vertex>& outVertices,
//                           std::vector<std::vector<uint32_t>>& outIndices,
//                           std::vector<Material>& outMaterials,
//                           std::vector<Image>& outTextures,
//                           std::vector<int>& outMatIndices) {
//     spdlog::info("Load file: {}", filepath);
//     tinyobj::attrib_t attrib;
//     std::vector<tinyobj::shape_t> shapes;
//     std::vector<tinyobj::material_t> materials;
//     std::string warn, err;
//
//     std::string dir = std::filesystem::path{filepath}.parent_path().string();
//     if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str(),
//                           dir.c_str())) {
//         spdlog::error("Failed to load: {}", warn + err);
//     }
//     spdlog::info("Shapes: {}", shapes.size());
//     spdlog::info("Materials: {}", materials.size());
//
//     int texCount = 0;
//     std::unordered_map<std::string, int> textureNames{};
//
//     outMaterials.resize(materials.size());
//     for (size_t i = 0; i < materials.size(); i++) {
//         spdlog::info("material: {}", materials[i].name);
//         auto& mat = materials[i];
//         outMaterials[i].ambient = {mat.ambient[0], mat.ambient[1], mat.ambient[2]};
//         outMaterials[i].diffuse = {mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]};
//         outMaterials[i].specular = {mat.specular[0], mat.specular[1], mat.specular[2]};
//         outMaterials[i].emission = {mat.emission[0], mat.emission[1], mat.emission[2]};
//
//         // diffuse
//         if (!mat.diffuse_texname.empty()) {
//             if (textureNames.contains(mat.diffuse_texname)) {
//                 outMaterials[i].diffuseTexture = textureNames[mat.diffuse_texname];
//             } else {
//                 outMaterials[i].diffuseTexture = texCount;
//                 textureNames[mat.diffuse_texname] = texCount;
//                 texCount++;
//             }
//         }
//         // specular
//         if (!mat.specular_texname.empty()) {
//             if (textureNames.contains(mat.specular_texname)) {
//                 outMaterials[i].specularTexture = textureNames[mat.specular_texname];
//             } else {
//                 outMaterials[i].specularTexture = texCount;
//                 textureNames[mat.specular_texname] = texCount;
//                 texCount++;
//             }
//         }
//         // alpha
//         if (!mat.alpha_texname.empty()) {
//             if (textureNames.contains(mat.alpha_texname)) {
//                 outMaterials[i].alphaTexture = textureNames[mat.alpha_texname];
//             } else {
//                 outMaterials[i].alphaTexture = texCount;
//                 textureNames[mat.alpha_texname] = texCount;
//                 texCount++;
//             }
//         }
//         // emission
//         if (!mat.emissive_texname.empty()) {
//             if (textureNames.contains(mat.emissive_texname)) {
//                 outMaterials[i].emissionTexture = textureNames[mat.emissive_texname];
//             } else {
//                 outMaterials[i].emissionTexture = texCount;
//                 textureNames[mat.emissive_texname] = texCount;
//                 texCount++;
//             }
//         }
//     }
//
//     outTextures.resize(texCount);
//     for (auto& [name, index] : textureNames) {
//         std::string path = name;
//         std::ranges::replace(path, '\\', '/');
//         path = dir + "/" + path;
//         spdlog::info("  Texture {}: {}", index, path);
//         outTextures[index] = Image{path};
//     }
//
//     std::unordered_map<Vertex, uint32_t> uniqueVertices;
//     outIndices.resize(shapes.size());
//     outMatIndices.resize(shapes.size());
//     for (int shapeID = 0; shapeID < shapes.size(); shapeID++) {
//         auto& shape = shapes[shapeID];
//         spdlog::info("  Shape {}", shape.name);
//         outMatIndices[shapeID] = shape.mesh.material_ids[0];
//         // std::set<int> uniqueMaterialIDs;
//         // for (auto id : shape.mesh.material_ids) {
//         //     uniqueMaterialIDs.insert(id);
//         // }
//         // outMatIndices[shapeID] = *uniqueMaterialIDs.begin();
//
//         for (const auto& index : shape.mesh.indices) {
//             Vertex vertex;
//             vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
//                           -attrib.vertices[3 * index.vertex_index + 1],
//                           attrib.vertices[3 * index.vertex_index + 2]};
//             if (index.normal_index != -1) {
//                 vertex.normal = {attrib.normals[3 * index.normal_index + 0],
//                                  -attrib.normals[3 * index.normal_index + 1],
//                                  attrib.normals[3 * index.normal_index + 2]};
//             }
//             if (index.texcoord_index != -1) {
//                 vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
//                                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
//             }
//             if (!uniqueVertices.contains(vertex)) {
//                 outVertices.push_back(vertex);
//                 uniqueVertices[vertex] = uniqueVertices.size();
//             }
//             outIndices[shapeID].push_back(uniqueVertices[vertex]);
//         }
//     }
// }
