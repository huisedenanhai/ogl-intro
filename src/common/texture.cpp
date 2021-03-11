#include "texture.hpp"
#include <sstream>
#include <stb_image.h>

Texture2D::Texture2D(const fs::path &name, TextureSettings *settings) {
  auto full_path = Data::resolve(name).string();
  stbi_set_flip_vertically_on_load(true);
  int width, height, channels;
  unsigned char *data =
      stbi_load(full_path.c_str(), &width, &height, &channels, 0);
  if (!data) {
    std::stringstream ss;
    ss << "failed to load image " << full_path;
    throw std::runtime_error(ss.str());
  }
  init(data, GL_UNSIGNED_BYTE, width, height, channels, settings);

  stbi_image_free(data);
}

void Texture2D::init(uint8_t *data,
                     GLenum data_type,
                     int width,
                     int height,
                     GLenum internal_format,
                     GLenum format,
                     TextureSettings *settings) {
  _width = width;
  _height = height;
  TextureSettings default_settings{};

  if (settings == nullptr) {
    settings = &default_settings;
  }
  glGenTextures(1, &_tex_id);
  glBindTexture(GL_TEXTURE_2D, _tex_id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, settings->wrap_s);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, settings->wrap_t);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, settings->min_filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, settings->max_filter);
  glTexImage2D(GL_TEXTURE_2D,
               0,
               internal_format,
               _width,
               _height,
               0,
               format,
               data_type,
               data);
  glGenerateMipmap(GL_TEXTURE_2D);
}

void Texture2D::init(uint8_t *data,
                     GLenum data_type,
                     int width,
                     int height,
                     int channels,
                     TextureSettings *settings) {
  GLenum format = GL_RGBA;
  if (channels == 1) {
    format = GL_R;
  }
  if (channels == 2) {
    format = GL_RG;
  }
  if (channels == 3) {
    format = GL_RGB;
  }
  init(data, data_type, width, height, format, format, settings);
}

Texture2D::Texture2D(uint8_t *data,
                     GLenum data_type,
                     int width,
                     int height,
                     int channels,
                     TextureSettings *settings) {
  init(data, data_type, width, height, channels, settings);
}

Texture2D::Texture2D(uint8_t *data,
                     GLenum data_type,
                     int width,
                     int height,
                     GLenum internal_format,
                     GLenum format,
                     TextureSettings *settings) {
  init(data, data_type, width, height, internal_format, format, settings);
}

Texture2D::~Texture2D() {
  glDeleteTextures(1, &_tex_id);
}

GLuint Texture2D::get() const {
  return _tex_id;
}

int Texture2D::width() const {
  return _width;
}

int Texture2D::height() const {
  return _height;
}
