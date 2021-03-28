#include "gltf.hpp"
#include "data.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <sstream>
#include <tiny_gltf.h>

Gltf::Gltf(const fs::path &name) {
  load_model(name);
}

void Gltf::load_model(const fs::path &name) {
  tinygltf::TinyGLTF loader;
  tinygltf::Model model;
  std::string err;
  std::string warn;

  auto model_path = Data::resolve(name);
  bool ret = false;
  auto extension = model_path.extension();
  auto model_path_str = model_path.string();
  if (extension == ".gltf") {
    ret = loader.LoadASCIIFromFile(&model, &err, &warn, model_path_str.c_str());
  } else if (extension == ".glb") {
    ret =
        loader.LoadBinaryFromFile(&model, &err, &warn, model_path_str.c_str());
  } else {
    std::stringstream ss;
    ss << "invalid file extension for GLTF: " << extension
       << ", should be .gltf or .glb";
    throw std::runtime_error(ss.str());
  }
  if (!warn.empty()) {
    std::cout << "Warn: " << warn << std::endl;
  }
  if (!err.empty()) {
    throw std::runtime_error("failed to load " + model_path.string() + ": " +
                             err);
  }
  if (!ret) {
    throw std::runtime_error("failed to parse " + model_path.string());
  }

  if (model.scenes.size() == 0) {
    return;
  }

  load_meshes(model);
  load_textures(model);
  load_materials(model);
  load_scene(model);
}

void Gltf::load_materials(tinygltf::Model &model) {
  auto tex = [](int index, int default_index) {
    return index < 0 ? default_index : index;
  };
  for (auto &mat : model.materials) {
    auto &pbr = mat.pbrMetallicRoughness;
    auto m = std::make_unique<Material>();
    m->base_color = tex(pbr.baseColorTexture.index, _white_tex_index);
    m->base_color_factor = glm::vec4(pbr.baseColorFactor[0],
                                     pbr.baseColorFactor[1],
                                     pbr.baseColorFactor[2],
                                     pbr.baseColorFactor[3]);
    m->metallic_factor = (float)pbr.metallicFactor;
    m->roughness_factor = (float)pbr.roughnessFactor;
    m->metallic_roughness =
        tex(pbr.metallicRoughnessTexture.index, _white_tex_index);
    m->normal = tex(mat.normalTexture.index, _default_normal_tex_index);
    m->normal_scale = (float)mat.normalTexture.scale;
    m->occlusion = tex(mat.occlusionTexture.index, _white_tex_index);
    m->occlusion_strength = (float)mat.occlusionTexture.strength;
    m->emission = tex(mat.emissiveTexture.index, _white_tex_index);
    m->emission_factor = glm::vec3(
        mat.emissiveFactor[0], mat.emissiveFactor[1], mat.emissiveFactor[2]);

    m->mode = Material::Opaque;
    if (mat.alphaMode == "BLEND") {
      m->mode = Material::Blend;
    }
    m->double_sided = mat.doubleSided;

    materials.emplace_back(std::move(m));
  }
}

void Gltf::load_textures(tinygltf::Model &model) {
  // All textures are loaded linearly. Do gamma correction in shader if
  // necessary
  for (auto &tex : model.textures) {
    auto &sampler = model.samplers[tex.sampler];
    auto &image = model.images[tex.source];
    int width = image.width;
    int height = image.height;
    int channels = image.component;
    GLenum type = GL_UNSIGNED_BYTE;
    if (image.bits == 16) {
      type = GL_UNSIGNED_SHORT;
    }

    TextureSettings settings{};

    settings.wrap_s = sampler.wrapS;
    settings.wrap_t = sampler.wrapT;

    if (sampler.minFilter > 0) {
      settings.min_filter = sampler.minFilter;
    }
    if (sampler.magFilter > 0) {
      settings.max_filter = sampler.magFilter;
    }

    textures.push_back(
        std::make_unique<Texture2D>((uint8_t *)image.image.data(),
                                    type,
                                    width,
                                    height,
                                    channels,
                                    &settings));
  }
  auto add_default_tex = [&](uint8_t *color) {
    auto index = (uint32_t)(textures.size());
    textures.push_back(
        std::make_unique<Texture2D>(color, GL_UNSIGNED_BYTE, 1, 1, 4));
    return index;
  };
  uint8_t white[] = {255, 255, 255, 255};
  uint8_t normal[] = {128, 128, 255, 255};
  _white_tex_index = add_default_tex(white);
  _default_normal_tex_index = add_default_tex(normal);
}

void Gltf::load_meshes(tinygltf::Model &model) {
  auto make_reader = [&](int accessor_index) {
    auto &accessor = model.accessors[accessor_index];
    auto &buffer_view = model.bufferViews[accessor.bufferView];
    auto stride = accessor.ByteStride(buffer_view);
    auto offset = accessor.byteOffset + buffer_view.byteOffset;
    auto &buffer = model.buffers[buffer_view.buffer];
    return [offset, &buffer, stride](int index) {
      return &buffer.data[offset + index * stride];
    };
  };

  for (auto &mesh : model.meshes) {
    std::vector<Primitive> primitives;
    primitives.reserve(mesh.primitives.size());
    for (auto &prim : mesh.primitives) {
      std::vector<Mesh::Vertex> vertices;
      {
        auto copy_attr =
            [&](const std::string &attr_name, auto func, int accessor_type) {
              auto it = prim.attributes.find(attr_name);
              if (it == prim.attributes.end()) {
                return;
              }
              auto accessor_index = it->second;
              if (accessor_index < 0) {
                return;
              }
              auto accessor = model.accessors[accessor_index];
              if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
                std::cout << "warn: only support float vertex attribute"
                          << std::endl;
                return;
              }
              if (accessor.type != accessor_type) {
                std::cout << "warn: accessor type not surpport for attribute "
                          << attr_name << std::endl;
                return;
              }
              if (accessor.count > vertices.size()) {
                vertices.resize(accessor.count);
              }
              auto reader = make_reader(accessor_index);
              for (int i = 0; i < accessor.count; i++) {
                func(vertices[i], reader(i));
              }
            };

#define COPY_ATTR(name, field, type)                                           \
  copy_attr(                                                                   \
      name,                                                                    \
      [](Mesh::Vertex &vert, uint8_t *data) {                                  \
        std::memcpy(&vert.field, data, sizeof(Mesh::Vertex::field));           \
      },                                                                       \
      type)

        // for all attributes see
        // https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md
        // we only copy what we need
        COPY_ATTR("POSITION", position, TINYGLTF_TYPE_VEC3);
        COPY_ATTR("NORMAL", normal, TINYGLTF_TYPE_VEC3);
        COPY_ATTR("TANGENT", tangent, TINYGLTF_TYPE_VEC4);
        COPY_ATTR("TEXCOORD_0", uv0, TINYGLTF_TYPE_VEC2);
        COPY_ATTR("TEXCOORD_1", uv1, TINYGLTF_TYPE_VEC2);
        COPY_ATTR("COLOR_0", color, TINYGLTF_TYPE_VEC4);

#undef COPY_ATTR
      }

      std::vector<uint32_t> indices;
      {
        auto accessor_index = prim.indices;
        if (accessor_index >= 0) {
          auto &accessor = model.accessors[accessor_index];
          auto reader = make_reader(accessor_index);
          indices.resize(accessor.count);
          for (int i = 0; i < accessor.count; i++) {
            switch (accessor.componentType) {
            case TINYGLTF_COMPONENT_TYPE_BYTE:
              indices[i] = *(int8_t *)reader(i);
              break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
              indices[i] = *(uint8_t *)reader(i);
              break;
            case TINYGLTF_COMPONENT_TYPE_SHORT:
              indices[i] = *(int16_t *)reader(i);
              break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
              indices[i] = *(uint16_t *)reader(i);
              break;
            case TINYGLTF_COMPONENT_TYPE_INT:
              indices[i] = *(int32_t *)reader(i);
              break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
              indices[i] = *(uint32_t *)reader(i);
              break;
            default:
              throw std::runtime_error("invalid type for indices");
            }
          }
        }
      }

      if (indices.empty()) {
        primitives.emplace_back(Primitive{
            std::make_unique<Mesh>(
                vertices.data(), (uint32_t)vertices.size(), nullptr, 0),
            prim.material});
      } else {
        primitives.emplace_back(
            Primitive{std::make_unique<Mesh>(vertices.data(),
                                             (uint32_t)vertices.size(),
                                             indices.data(),
                                             (uint32_t)indices.size()),
                      prim.material});
      }
    }

    meshes.emplace_back(std::move(primitives));
  }
}

void Gltf::load_scene(tinygltf::Model &model) {
  auto scene_index = model.defaultScene < 0 ? 0 : model.defaultScene;
  auto &scene = model.scenes[scene_index];

  for (int node_index : scene.nodes) {
    auto &node = model.nodes[node_index];
    if (node.mesh < 0) {
      continue;
    }

    auto transform = glm::identity<glm::mat4>();
    if (node.rotation.size() == 4) {
      glm::quat rotation(node.rotation[0],
                         node.rotation[1],
                         node.rotation[2],
                         node.rotation[3]);
      transform = glm::mat4_cast(rotation) * transform;
    }
    if (node.scale.size() == 3) {
      transform = glm::scale(
          transform, glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
    }
    if (node.translation.size() == 3) {
      transform = glm::translate(transform,
                                 glm::vec3(node.translation[0],
                                           node.translation[1],
                                           node.translation[2]));
    }
    if (node.matrix.size() == 16) {
      std::memcpy(&transform, node.matrix.data(), sizeof(transform));
    }

    draws.push_back(MeshDraw{node.mesh, transform});
  }
}
