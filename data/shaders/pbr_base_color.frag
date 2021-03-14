#version 330 core

in vec3 position_vs;
in vec3 normal_vs;
in vec3 tangent_vs;
in vec3 bitangent_vs;
in vec2 uv0_vs;

layout(std140) uniform Params {
  vec4 base_color_factor;
  float metallic_factor;
  float roughness_factor;
  float normal_scale;
  float occlusion_strength;
  vec3 emission_factor;
  vec3 light_dir_vs;
  vec3 light_radiance;
  vec3 env_irradiance;
};

uniform sampler2D base_color_tex;
uniform sampler2D metallic_roughness_tex;
uniform sampler2D normal_tex;
uniform sampler2D occlusion_tex;
uniform sampler2D emission_tex;

layout(location = 0) out vec4 frag_color_out;

vec3 srgb_to_linear(vec3 srgb) {
  return pow(srgb, vec3(2.2));
}

vec4 srgb_to_linear(vec4 srgb) {
  return pow(srgb, vec4(2.2));
}

vec4 get_base_color() {
  vec4 raw = texture(base_color_tex, uv0_vs);
  return srgb_to_linear(raw) * base_color_factor;
}

void main() {
  vec4 base_color = get_base_color();
  frag_color_out = base_color;
}