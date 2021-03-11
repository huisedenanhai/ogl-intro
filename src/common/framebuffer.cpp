#include "framebuffer.hpp"

Framebuffer::Framebuffer(Texture2D **color_attachments,
                         uint32_t color_attachment_count,
                         Texture2D *depth_stencil_attachment) {
  init(color_attachments, color_attachment_count, depth_stencil_attachment);
}

GLuint Framebuffer::get() const {
  return _id;
}

void Framebuffer::init(Texture2D **color_attachments,
                       uint32_t color_attachment_count,
                       Texture2D *depth_stencil_attachment) {
  glGenFramebuffers(1, &_id);
  glBindFramebuffer(GL_FRAMEBUFFER, _id);

  for (uint32_t i = 0; i < color_attachment_count; i++) {
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0 + i,
                           GL_TEXTURE_2D,
                           color_attachments[i]->get(),
                           0);
  }

  if (depth_stencil_attachment != nullptr) {
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_DEPTH_STENCIL_ATTACHMENT,
                           GL_TEXTURE_2D,
                           depth_stencil_attachment->get(),
                           0);
  }

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    glDeleteFramebuffers(1, &_id);
    throw std::runtime_error("incomplete frame buffer");
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Framebuffer::~Framebuffer() {
  glDeleteFramebuffers(1, &_id);
}
