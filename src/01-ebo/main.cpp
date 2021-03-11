#include "../common/application.hpp"
#include "../common/shader.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

static const char *vertex_shader_text = R"(
#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec3 color;

out vec3 color_vs;

void main() {
  gl_Position = vec4(position, 0.0, 1.0);
  color_vs = color;
}
)";

static const char *fragment_shader_text = R"(
#version 330 core

in vec3 color_vs;

layout(location = 0) out vec4 color;

void main() {
  color = vec4(color_vs, 1.0);
}
)";

struct Vertex {
  float x, y;
  float r, g, b;
};

class EboApp final : public Application {
public:
  EboApp() : Application("Ebo", 800, 600) {}
  ~EboApp() {
    glDeleteVertexArrays(1, &_vao);
    glDeleteBuffers(1, &_vertex_buffer);
    glDeleteBuffers(1, &_index_buffer);
  }

private:
  void init_program() {
    auto vert_shader =
        std::make_unique<Shader>(vertex_shader_text, GL_VERTEX_SHADER, "vert");
    auto frag_shader = std::make_unique<Shader>(
        fragment_shader_text, GL_FRAGMENT_SHADER, "frag");
    GLuint shaders[] = {vert_shader->get(), frag_shader->get()};

    _program = std::make_unique<Program>(shaders, 2);

    // now shaders can be released automatically
  }

  void init() override {
    init_program();

    _vertices = std::vector<Vertex>({
        {-0.5f, -0.5f, 1.0f, 0.0f, 0.0f}, // left-down
        {0.5f, -0.5f, 0.0f, 1.0f, 0.0f},  // right-down
        {-0.5f, 0.5f, 0.0f, 0.0f, 1.0f},  // left-up
        {0.5f, 0.5f, 1.0f, 1.0f, 1.0f}    // right-up
    });

    _indices = std::vector<uint32_t>({0, 1, 2, 1, 3, 2});

    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    glGenBuffers(1, &_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(Vertex) * _vertices.size(),
                 _vertices.data(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, x));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, r));
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &_index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(uint32_t) * _indices.size(),
                 _indices.data(),
                 GL_STATIC_DRAW);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }

  void update() override {
    glClearColor(0.0, 0.0, 0.0, 1.0);

    int width, height;
    glfwGetFramebufferSize(_window, &width, &height);

    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(_program->get());
    glBindVertexArray(_vao);
    glDrawElements(
        GL_TRIANGLES, (GLsizei)_indices.size(), GL_UNSIGNED_INT, nullptr);
  }

  std::unique_ptr<Program> _program;

  GLuint _vao;
  GLuint _vertex_buffer;
  GLuint _index_buffer;

  std::vector<Vertex> _vertices{};
  std::vector<uint32_t> _indices{};
};

int main() {
  try {
    EboApp app{};
    app.run();
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}