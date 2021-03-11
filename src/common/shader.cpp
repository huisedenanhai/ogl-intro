#include "shader.hpp"
#include <optional>
#include <sstream>
#include <stdexcept>
#include <vector>

static GLuint compile_shader(const char *text, GLenum type, const char *name) {
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

    std::stringstream ss;
    ss << "failed to compile shader \""
       << (name == nullptr ? "<unknown>" : name) << "\": " << error_log.data();

    glDeleteShader(shader); // Don't leak the shader.
    throw std::runtime_error(ss.str());
  }
  return shader;
}

Shader::Shader(const char *text, GLenum stage, const char *name) {
  _id = compile_shader(text, stage, name);
}

Shader::Shader(const fs::path &name, GLenum stage) {
  auto data = Data::load(name);
  data.push_back(0);
  _id = compile_shader((const char *)data.data(), stage, name.string().c_str());
}

Shader::~Shader() {
  glDeleteShader(_id);
}

GLuint Shader::get() const {
  return _id;
}

static GLuint link_program(const GLuint *shaders, uint32_t shader_count) {
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

    std::stringstream ss;
    ss << "failed to link program: " << error_log.data();
    glDeleteProgram(program);
    throw std::runtime_error(ss.str());
  }
  return program;
}

Program::Program(const GLuint *shaders, uint32_t count) {
  _id = link_program(shaders, count);
}

Program::~Program() {
  glDeleteProgram(_id);
}

GLuint Program::get() const {
  return _id;
}

std::unique_ptr<Program> Program::create_from_source(const char *vert_source,
                                                     const char *frag_source) {
  auto vert_shader =
      std::make_unique<Shader>(vert_source, GL_VERTEX_SHADER, "vert");
  auto frag_shader =
      std::make_unique<Shader>(frag_source, GL_FRAGMENT_SHADER, "frag");
  GLuint shaders[] = {vert_shader->get(), frag_shader->get()};

  return std::make_unique<Program>(shaders, 2);
}

std::unique_ptr<Program> Program::create_from_files(const fs::path &vert_file,
                                                    const fs::path &frag_file) {
  auto vert_shader = std::make_unique<Shader>(vert_file, GL_VERTEX_SHADER);
  auto frag_shader = std::make_unique<Shader>(frag_file, GL_FRAGMENT_SHADER);
  GLuint shaders[] = {vert_shader->get(), frag_shader->get()};

  return std::make_unique<Program>(shaders, 2);
}
