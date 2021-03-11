#pragma once

#include "data.hpp"
#include <GL/glew.h>

class Shader {
public:
  Shader(const char *text, GLenum stage, const char *name = nullptr);
  Shader(const fs::path &name, GLenum stage);
  ~Shader();

  GLuint get() const;

private:
  GLuint _id{};
};

class Program {
public:
  Program(const GLuint *shaders, uint32_t count);
  ~Program();

  static std::unique_ptr<Program> create_from_source(const char *vert_source,
                                                     const char *frag_source);
  static std::unique_ptr<Program> create_from_files(const fs::path &vert_file,
                                                    const fs::path &frag_file);

  GLuint get() const;

private:
  void init(GLuint *shaders, uint32_t count);
  GLuint _id{};
};