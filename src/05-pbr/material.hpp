#pragma once

#include "../common/renderer.hpp"
#include "../common/shader.hpp"
#include "../common/texture.hpp"

class PbrMaterial : public IMaterial {
public:
  enum Mode { Opaque, Blend };

  Mode mode;
  bool double_sided;
  Texture2D *base_color;
  glm::vec4 base_color_factor;
  float metallic_factor;
  float roughness_factor;
  Texture2D *metallic_roughness;
  Texture2D *normal;
  float normal_scale;
  Texture2D *occlusion;
  float occlusion_strength;
  Texture2D *emission;
  glm::vec3 emission_factor;
  Texture2D *lut;

  glm::vec3 light_dir_vs;
  glm::vec3 light_radiance;
  glm::vec3 env_radiance;

  explicit PbrMaterial(bool show_base_color);
  void use() override;

private:
  std::unique_ptr<Program> _program;
  GLint _base_color_location;
  GLint _metallic_roughness_location;
  GLint _normal_location;
  GLint _occlusion_location;
  GLint _emission_location;
  GLint _lut_location;

  std::unique_ptr<Buffer> _transform_buffer;
  std::unique_ptr<Buffer> _params_buffer;
};

class ToneMappingMaterial : public IMaterial {
public:
  float exposure = 1.0f;

  ToneMappingMaterial();
  void use() override;

private:
  std::unique_ptr<Program> _program;
  GLint _transform_location;
  GLint _image_location;
  GLint _exposure_location;
};

class PrecomputeEnvBrdfMaterial : public IMaterial {
public:
  PrecomputeEnvBrdfMaterial();
  void use() override;

private:
  std::unique_ptr<Program> _program;
  GLint _transform_location;
};
