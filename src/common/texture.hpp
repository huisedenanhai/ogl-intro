#pragma once

#include "data.hpp"
#include <GL/glew.h>
#include <glm/glm.hpp>

struct TextureSettings {
  GLenum wrap_s = GL_MIRRORED_REPEAT;
  GLenum wrap_t = GL_MIRRORED_REPEAT;
  glm::vec4 border_color = glm::vec4(0.0, 0.0, 0.0, 0.0);
  GLenum min_filter = GL_LINEAR_MIPMAP_LINEAR;
  GLenum max_filter = GL_LINEAR;
};

class Texture2D {
public:
  Texture2D(const fs::path &name, TextureSettings *settings = nullptr);
  Texture2D(uint8_t *data,
            GLenum data_type,
            int width,
            int height,
            int channels,
            TextureSettings *settings = nullptr);

  Texture2D(uint8_t *data,
            GLenum data_type,
            int width,
            int height,
            GLenum internal_format,
            GLenum format,
            TextureSettings *settings = nullptr);

  ~Texture2D();

  GLuint get() const;

  int width() const;
  int height() const;

private:
  GLuint _tex_id;
  int _width, _height;

  void init(uint8_t *data,
            GLenum data_type,
            int width,
            int height,
            int channels,
            TextureSettings *settings = nullptr);

  void init(uint8_t *data,
            GLenum data_type,
            int width,
            int height,
            GLenum internal_format,
            GLenum format,
            TextureSettings *settings = nullptr);
};