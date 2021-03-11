#include <GL/glew.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <iostream>
#include <optional>
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

static const char *fragment_shader_text =
    R"(
#version 330 core

in vec3 color_vs;

layout(location = 0) out vec4 color;

void main() {
  color = vec4(color_vs, 1.0);
}
)";

std::optional<GLuint> compile_shader(const char *text, GLenum type) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &text, NULL);
  glCompileShader(shader);

  GLint is_compiled = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &is_compiled);
  if (is_compiled == GL_FALSE) {
    GLint max_length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_length);

    // The maxLength includes the NULL character
    std::vector<GLchar> error_log(max_length);
    glGetShaderInfoLog(shader, max_length, &max_length, &error_log[0]);

    std::cerr << error_log.data() << std::endl;

    glDeleteShader(shader); // Don't leak the shader.
    return std::nullopt;
  }
  return shader;
}

std::optional<GLuint> link_program(GLuint *shaders, uint32_t shader_count) {
  GLuint program = glCreateProgram();
  for (uint32_t i = 0; i < shader_count; i++) {
    glAttachShader(program, shaders[i]);
  }
  glLinkProgram(program);

  int success;
  // check for linking errors
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    GLint max_length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &max_length);
    std::vector<GLchar> error_log(max_length);
    glGetProgramInfoLog(program, max_length, &max_length, &error_log[0]);

    std::cout << error_log.data() << std::endl;
    glDeleteProgram(program);
    return std::nullopt;
  }
  return program;
}

GLFWwindow *init() {
  if (glfwInit() == GLFW_FALSE) {
    throw std::runtime_error("failed to init glfw");
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__ // for macos
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  GLFWwindow *window = glfwCreateWindow(800, 600, "Hello", nullptr, nullptr);
  if (!window) {
    throw std::runtime_error("failed to create window");
  }
  glfwMakeContextCurrent(window);

  GLenum e = glewInit();
  if (e != GLEW_OK) {
    glfwDestroyWindow(window);
    std::cerr << glewGetErrorString(e) << std::endl;
    throw std::runtime_error("failed to init glew");
  }
  return window;
}

struct Vertex {
  float x, y;
  float r, g, b;
};

int main() {
  GLFWwindow *window = init();
  GLuint vertex_shader =
      compile_shader(vertex_shader_text, GL_VERTEX_SHADER).value();
  GLuint fragment_shader =
      compile_shader(fragment_shader_text, GL_FRAGMENT_SHADER).value();

  GLuint shaders[] = {vertex_shader, fragment_shader};
  GLuint program = link_program(shaders, 2).value();

  Vertex vertices[] = {
      {-0.5f, -0.5f, 1.0f, 0.0f, 0.0f}, // left
      {0.5f, -0.5f, 0.0f, 1.0f, 0.0f},  // right
      {0.0f, 0.5f, 0.0f, 0.0f, 1.0f}    // top
  };

  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  GLuint vertex_buffer;
  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glVertexAttribPointer(
      0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, x));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(
      1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, r));
  glEnableVertexAttribArray(1);

  glClearColor(1.0, 1.0, 1.0, 1.0);
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    glfwSwapBuffers(window);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
  }

  glDeleteBuffers(1, &vertex_buffer);
  glDeleteVertexArrays(1, &vao);

  glfwDestroyWindow(window);
  glfwTerminate();
}