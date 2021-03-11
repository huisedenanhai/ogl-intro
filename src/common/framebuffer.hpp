#pragma once
#include "texture.hpp"

class Framebuffer {
public:
  Framebuffer(Texture2D **color_attachments,
              uint32_t color_attachment_count,
              Texture2D *depth_stencil_attachment);

  ~Framebuffer();

  GLuint get() const;

private:
  GLuint _id;

  void init(Texture2D **color_attachments,
            uint32_t color_attachment_count,
            Texture2D *depth_stencil_attachment);
};