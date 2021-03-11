#include "material.hpp"

static const char *vertex_source = R"(
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

static const char *fragment_base_color_source = R"(
#version 330 core

in vec2 uv_fs;

uniform sampler2D base_color;

layout(location = 0) out vec4 frag_color;

void main() {
  frag_color = vec4(texture(base_color, uv_fs).rgb, 1.0);
}
)";

static const char *fragment_chromatic_aberration_source = R"(
#version 330 core

in vec2 uv_fs;

uniform sampler2D base_color;
uniform float strength;

layout(location = 0) out vec4 frag_color;

void main() {
  frag_color = vec4(
    texture(base_color, uv_fs + vec2(strength, 0.0)).r,
    texture(base_color, uv_fs).g,
    texture(base_color, uv_fs + vec2(-strength, 0.0)).b,
    1.0);
}
)";

BaseColorMaterial::BaseColorMaterial() {
  _program =
      Program::create_from_source(vertex_source, fragment_base_color_source);
  _transform_location = glGetUniformLocation(_program->get(), "transform");
  _image_location = glGetUniformLocation(_program->get(), "base_color");
}

void BaseColorMaterial::use() {
  glUseProgram(_program->get());
  glUniformMatrix4fv(_transform_location, 1, false, (GLfloat *)&transform);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, base_tex != nullptr ? base_tex->get() : 0);
  glUniform1i(_image_location, 0);
}

ChromaticAberrationMaterial::ChromaticAberrationMaterial() {
  _program = Program::create_from_source(vertex_source,
                                         fragment_chromatic_aberration_source);
  _transform_location = glGetUniformLocation(_program->get(), "transform");
  _image_location = glGetUniformLocation(_program->get(), "base_color");
  _strength_location = glGetUniformLocation(_program->get(), "strength");
}

void ChromaticAberrationMaterial::use() {
  glUseProgram(_program->get());
  glUniformMatrix4fv(_transform_location, 1, false, (GLfloat *)&transform);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, base_tex != nullptr ? base_tex->get() : 0);
  glUniform1i(_image_location, 0);
  glUniform1f(_strength_location, strength);
}
