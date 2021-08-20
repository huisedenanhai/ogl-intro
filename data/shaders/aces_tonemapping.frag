#version 330 core

in vec2 uv_fs;

uniform sampler2D main_tex;
uniform float exposure;

layout(location = 0) out vec4 frag_color;

vec3 aces_approx(vec3 v) {
  float a = 2.51;
  float b = 0.03;
  float c = 2.43;
  float d = 0.59;
  float e = 0.14;
  return (v * (a * v + b)) / (v * (c * v + d) + e);
}

void main() {
  vec3 hdr = texture(main_tex, uv_fs).rgb;
  vec3 ldr = aces_approx(hdr * exposure);
  vec3 gamma_corrected = pow(ldr, vec3(1.0 / 2.2));
  frag_color = vec4(gamma_corrected, 1.0);
}