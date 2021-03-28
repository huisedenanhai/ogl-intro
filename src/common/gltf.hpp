#pragma once

#include "data.hpp"
#include "mesh.hpp"
#include "texture.hpp"
#include <memory>

namespace tinygltf {
class Model;
}

class Gltf {
public:
  Gltf(const fs::path &name);

  struct Primitive {
    std::unique_ptr<Mesh> mesh;
    int material;
  };

  struct MeshDraw {
    int index;
    glm::mat4 transform;
  };

  struct Material {
    enum Mode { Opaque, Blend };
    Mode mode;
    bool double_sided;
    glm::vec4 base_color_factor;
    int base_color;
    float metallic_factor;
    float roughness_factor;
    int metallic_roughness;
    int normal;
    float normal_scale;
    int occlusion;
    float occlusion_strength;
    int emission;
    glm::vec3 emission_factor;
  };

  std::vector<std::vector<Primitive>> meshes;
  std::vector<MeshDraw> draws;
  std::vector<std::unique_ptr<Texture2D>> textures;
  std::vector<std::unique_ptr<Material>> materials;

private:
  void load_model(const fs::path &name);
  void load_materials(tinygltf::Model &model);
  void load_textures(tinygltf::Model &model);
  void load_meshes(tinygltf::Model &model);
  void load_scene(tinygltf::Model &model);

  uint32_t _white_tex_index;
  uint32_t _default_normal_tex_index;
};