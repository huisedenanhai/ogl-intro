#include "renderer.hpp"
#include <glm/gtc/matrix_transform.hpp>

Renderer::Renderer() {
  init_bilt();
}

void Renderer::init_bilt() {
  // draw a big triangle that cover the full screen
  std::vector<Mesh::Vertex> vertices = {
      {{-1.0f, -1.0f, 0.0f}, {}, {}, {0.0f, 0.0f}}, // left-down
      {{3.0f, -1.0f, 0.0f}, {}, {}, {2.0f, 0.0f}},  // right-down
      {{-1.0f, 3.0f, 0.0f}, {}, {}, {0.0f, 2.0f}},  // left-up
  };

  _full_screen_triangle =
      std::make_unique<Mesh>(vertices.data(), vertices.size(), nullptr, 0);
}

void Renderer::bilt(Texture2D *tex, IMaterial *material) {
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_DEPTH_TEST);
  material->model = glm::identity<glm::mat4>();
  material->view = glm::identity<glm::mat4>();
  material->projection = glm::identity<glm::mat4>();
  material->main_tex = tex;
  material->use();
  _full_screen_triangle->draw();
}
