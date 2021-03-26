#pragma once

#include "mesh.hpp"
#include "texture.hpp"
#include <glm/glm.hpp>

class IMaterial {
public:
  glm::mat4 model{};
  glm::mat4 view{};
  glm::mat4 projection{};
  Texture2D *main_tex{};

  virtual void use() = 0;
};

class Renderer {
public:
  Renderer();

  void blit(Texture2D *tex, IMaterial *material);

private:
  void init_blit();

  std::unique_ptr<Mesh> _full_screen_triangle;
};