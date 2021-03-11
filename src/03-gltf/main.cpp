#include "../common/application.hpp"
#include "../common/gltf.hpp"
#include "../common/shader.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <iostream>
#include <tiny_gltf.h>
#include <vector>

static const char *vertex_shader_text = R"(
#version 330 core

layout(location = 0) in vec3 position;
layout(location = 3) in vec2 uv;

uniform mat4 transform;

out vec2 uv_fs;

void main() {
  gl_Position = transform * vec4(position, 1.0);
  uv_fs = uv;
}
)";

static const char *fragment_shader_text = R"(
#version 330 core

in vec2 uv_fs;

uniform sampler2D base_color;

layout(location = 0) out vec4 frag_color;

void main() {
  frag_color = vec4(texture(base_color, uv_fs).rgb, 1.0);
}
)";

class GltfApp final : public Application {
public:
  GltfApp() : Application("GLTF", 800, 600) {}

private:
  void init() override {
    _program =
        Program::create_from_source(vertex_shader_text, fragment_shader_text);
    _camera = std::make_unique<ModelViewerCamera>();
    _scene = std::make_unique<Gltf>("FlightHelmet/FlightHelmet.gltf");

    // get uniform location by name
    _transform_location = glGetUniformLocation(_program->get(), "transform");
    _image_location = glGetUniformLocation(_program->get(), "base_color");
  }

  void draw_ui() {
    ImGui::Text("Camera");
    _camera->draw_ui();
  }

  void draw() {
    glClearColor(0.0, 0.0, 0.0, 1.0);

    int width, height;
    glfwGetFramebufferSize(_window, &width, &height);

    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(_program->get());
    float aspect = (float)width / (float)height;

    glm::mat4 view = _camera->view();
    glm::mat4 projection = _camera->projection(aspect);

    for (auto &draw : _scene->draws) {
      auto transform = projection * view * draw.transform;
      glUniformMatrix4fv(_transform_location, 1, false, (GLfloat *)&transform);
      for (auto &prim : _scene->meshes[draw.index]) {
        auto *mat = _scene->materials[prim.material].get();
        auto *base_tex = _scene->textures[mat->base_color].get();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, base_tex->get());
        glUniform1i(_image_location, 0);

        prim.mesh->draw();
      }
    }
  }

  void update() override {
    draw_ui();
    draw();
  }

  std::unique_ptr<Program> _program;

  GLint _transform_location;
  GLint _image_location;

  std::unique_ptr<ModelViewerCamera> _camera;
  std::unique_ptr<Gltf> _scene;
};

int main() {
  try {
    GltfApp app{};
    app.run();
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}