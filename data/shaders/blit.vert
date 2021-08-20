#version 330 core

layout(location = 0) in vec3 position;
layout(location = 3) in vec2 uv;

uniform mat4 transform;

out vec2 uv_fs;

void main() {
  gl_Position = transform * vec4(position, 1.0);
  uv_fs = uv;
}
