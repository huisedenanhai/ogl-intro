#pragma once

#include "../common/shader.hpp"
#include "../common/texture.hpp"

class BaseColorMaterial {
public:
  glm::mat4 transform;
  Texture2D *base_tex = nullptr;

  BaseColorMaterial();
  void use();

private:
  std::unique_ptr<Program> _program;
  GLint _transform_location;
  GLint _image_location;
};

class ChromaticAberrationMaterial {
public:
  glm::mat4 transform{};
  Texture2D *base_tex = nullptr;
  float strength = 0.02f;

  ChromaticAberrationMaterial();
  void use();

private:
  std::unique_ptr<Program> _program;
  GLint _transform_location;
  GLint _image_location;
  GLint _strength_location;
};
