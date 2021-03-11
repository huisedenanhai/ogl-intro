#include "../common/application.hpp"
#include "../common/mesh.hpp"
#include "../common/shader.hpp"
#include "../common/texture.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>
#include <iostream>
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

uniform vec3 color_uniform;
uniform sampler2D image;

layout(location = 0) out vec4 frag_color;

void main() {
  frag_color = vec4(color_uniform * texture(image, uv_fs).rgb, 1.0);
}
)";

class UniformApp final : public Application {
public:
  UniformApp() : Application("Uniform", 800, 600) {}

private:
  void init() override {
    _program =
        Program::create_from_source(vertex_shader_text, fragment_shader_text);

    std::vector<Mesh::Vertex> vertices = {
        {{-0.5f, -0.5f, 0.0f}, {}, {}, {0.0f, 0.0f}}, // left-down
        {{0.5f, -0.5f, 0.0f}, {}, {}, {1.0f, 0.0f}},  // right-down
        {{-0.5f, 0.5f, 0.0f}, {}, {}, {0.0f, 1.0f}},  // left-up
        {{0.5f, 0.5f, 0.0f}, {}, {}, {1.0f, 1.0f}}    // right-up
    };

    std::vector<uint32_t> indices = {0, 1, 2, 1, 3, 2};

    _mesh = std::make_unique<Mesh>(vertices.data(),
                                   (uint32_t)vertices.size(),
                                   indices.data(),
                                   (uint32_t)indices.size());

    // see common/texture.cpp for more on texture loading
    _tex = std::make_unique<Texture2D>("texture.jpg");

    // get uniform location by name
    _color_location = glGetUniformLocation(_program->get(), "color_uniform");
    _transform_location = glGetUniformLocation(_program->get(), "transform");
    _image_location = glGetUniformLocation(_program->get(), "image");
  }

  glm::mat4 calculate_transform(float aspect) {
    glm::mat4 trans = glm::identity<glm::mat4>();
    trans = glm::rotate(trans, _rotation_rad, glm::vec3(0, 0, 1.0f));
    trans = glm::ortho(-aspect, aspect, -1.0f, 1.0f) * trans;
    return trans;
  }

  void draw_ui() {
    ImGui::SliderAngle("Rotation", &_rotation_rad);
    ImGui::ColorEdit3("Color", (float *)&_color);
  }

  void draw() {
    glClearColor(0.0, 0.0, 0.0, 1.0);

    int width, height;
    glfwGetFramebufferSize(_window, &width, &height);

    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(_program->get());
    auto transform = calculate_transform((float)width / (float)height);
    // update uniforms
    glUniformMatrix4fv(_transform_location, 1, false, (GLfloat *)&transform);

    glUniform3fv(_color_location, 1, (GLfloat *)&_color);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _tex->get());
    glUniform1i(_image_location, 0);

    _mesh->draw();
  }

  void update() override {
    draw_ui();
    draw();
  }

  std::unique_ptr<Program> _program;

  GLint _transform_location;
  GLint _color_location;
  GLint _image_location;

  float _rotation_rad = 0.0f;
  glm::vec3 _color{1.0f, 1.0f, 1.0f};
  std::unique_ptr<Mesh> _mesh;
  std::unique_ptr<Texture2D> _tex;
};

int main() {
  try {
    UniformApp app{};
    app.run();
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}