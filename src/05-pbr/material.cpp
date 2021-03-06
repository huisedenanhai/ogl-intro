#include "material.hpp"

namespace {
struct TransformBlock {
  glm::mat4 MV;
  glm::mat4 I_MV;
  glm::mat4 P;
};

struct ParamsBlock {
  glm::vec4 base_color_factor;
  float metallic_factor;
  float roughness_factor;
  float normal_scale;
  float occlusion_strength;
  glm::vec4 emission_factor; // use glm::vec4 for padding
  glm::vec4 light_dir_vs;    // use glm::vec4 for padding
  glm::vec4 light_radiance;  // use glm::vec4 for padding
  glm::vec4 env_radiance;    // use glm::vec4 for padding
};
} // namespace

PbrMaterial::PbrMaterial(bool show_base_color) {
  const char *frag_file =
      show_base_color ? "shaders/pbr_base_color.frag" : "shaders/pbr.frag";
  _program = Program::create_from_files("shaders/pbr.vert", frag_file);

#define INIT_UNIFORM_LOCATION(name)                                            \
  _##name##_location = glGetUniformLocation(_program->get(), #name "_tex")

  INIT_UNIFORM_LOCATION(base_color);
  INIT_UNIFORM_LOCATION(metallic_roughness);
  INIT_UNIFORM_LOCATION(normal);
  INIT_UNIFORM_LOCATION(occlusion);
  INIT_UNIFORM_LOCATION(emission);
  INIT_UNIFORM_LOCATION(lut);

#undef INIT_UNIFORM_LOCATION

  GLuint transform_index = glGetUniformBlockIndex(_program->get(), "Transform");
  glUniformBlockBinding(_program->get(), transform_index, 0);
  GLuint params_index = glGetUniformBlockIndex(_program->get(), "Params");
  glUniformBlockBinding(_program->get(), params_index, 1);

  _transform_buffer = std::make_unique<Buffer>(nullptr, sizeof(TransformBlock));
  _params_buffer = std::make_unique<Buffer>(nullptr, sizeof(ParamsBlock));
}

void PbrMaterial::use() {
  glUseProgram(_program->get());

#define ASSIGN_TEXTURE(index, name)                                            \
  glActiveTexture(GL_TEXTURE0 + index);                                        \
  glBindTexture(GL_TEXTURE_2D, name != nullptr ? name->get() : 0);             \
  glUniform1i(_##name##_location, index)

  ASSIGN_TEXTURE(0, base_color);
  ASSIGN_TEXTURE(1, metallic_roughness);
  ASSIGN_TEXTURE(2, normal);
  ASSIGN_TEXTURE(3, occlusion);
  ASSIGN_TEXTURE(4, emission);
  ASSIGN_TEXTURE(5, lut);

#undef ASSIGN_TEXTURE

  // update buffer data
  TransformBlock transform_block{};
  transform_block.MV = view * model;
  transform_block.I_MV = glm::inverse(transform_block.MV);
  transform_block.P = projection;
  glBindBuffer(GL_UNIFORM_BUFFER, _transform_buffer->get());
  glBufferSubData(
      GL_UNIFORM_BUFFER, 0, sizeof(TransformBlock), &transform_block);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  ParamsBlock params_block{};
  params_block.base_color_factor = base_color_factor;
  params_block.emission_factor = glm::vec4(emission_factor, 1.0f);
  params_block.occlusion_strength = occlusion_strength;
  params_block.normal_scale = normal_scale;
  params_block.metallic_factor = metallic_factor;
  params_block.roughness_factor = roughness_factor;

  params_block.light_dir_vs = glm::vec4(light_dir_vs, 0.0f);
  params_block.light_radiance = glm::vec4(light_radiance, 1.0f);
  params_block.env_radiance = glm::vec4(env_radiance, 1.0f);

  glBindBuffer(GL_UNIFORM_BUFFER, _params_buffer->get());
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ParamsBlock), &params_block);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  glBindBufferBase(GL_UNIFORM_BUFFER, 0, _transform_buffer->get());
  glBindBufferBase(GL_UNIFORM_BUFFER, 1, _params_buffer->get());

  if (double_sided) {
    glDisable(GL_CULL_FACE);
  } else {
    glEnable(GL_CULL_FACE);
  }
}

ToneMappingMaterial::ToneMappingMaterial() {
  _program = Program::create_from_files("shaders/blit.vert",
                                        "shaders/aces_tonemapping.frag");
  _transform_location = glGetUniformLocation(_program->get(), "transform");
  _image_location = glGetUniformLocation(_program->get(), "main_tex");
  _exposure_location = glGetUniformLocation(_program->get(), "exposure");
}

void ToneMappingMaterial::use() {
  glUseProgram(_program->get());
  glm::mat4 transform = projection * model * view;
  glUniformMatrix4fv(_transform_location, 1, false, (GLfloat *)&transform);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, main_tex != nullptr ? main_tex->get() : 0);
  glUniform1i(_image_location, 0);
  glUniform1f(_exposure_location, exposure);
}

PrecomputeEnvBrdfMaterial::PrecomputeEnvBrdfMaterial() {
  _program = Program::create_from_files("shaders/blit.vert",
                                        "shaders/pre_compute_env_brdf.frag");
  _transform_location = glGetUniformLocation(_program->get(), "transform");
}

void PrecomputeEnvBrdfMaterial::use() {
  glUseProgram(_program->get());
  glm::mat4 transform = projection * model * view;
  glUniformMatrix4fv(_transform_location, 1, false, (GLfloat *)&transform);
}
