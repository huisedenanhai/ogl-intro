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

vec3 decode_normal_ts() {
  vec3 normal = texture(normal_tex, uv0_vs).xyz * 2.0 - 1.0;
  return normalize(normal * vec3(normal_scale, normal_scale, 1.0));
}

vec3 get_normal_vs() {
  vec3 normal_ts = decode_normal_ts();
  return normal_ts.x * normalize(tangent_vs) +
         normal_ts.y * normalize(bitangent_vs) +
         normal_ts.z * normalize(normal_vs);
}

vec3 occlude_color(vec3 unocclude_color) {
  float occlusion = texture(occlusion_tex, uv0_vs).r;
  return mix(unocclude_color, unocclude_color * occlusion, occlusion_strength);
}

vec3 get_emission() {
  return srgb_to_linear(texture(emission_tex, uv0_vs).xyz) * emission_factor;
}

// use a simplified lighting model of filament, not optimized.
// see documentation of filament for a much more in depth explanation
// https://github.com/google/filament
// We use the traditional radiance unit, which should be scaled by PI to be in
// SI. Our BSDF thus differs from that of filament, which use SI, in a factor
// PI.
float D_GGX(float NoH, float roughness) {
  float a = NoH * roughness;
  float k = roughness / (1.0 - NoH * NoH + a * a);
  return k * k;
}

float V_SmithGGXCorrelated(float NoV, float NoL, float roughness) {
  float a2 = roughness * roughness;
  float GGXV = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
  float GGXL = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
  return 0.5 / (GGXV + GGXL);
}

vec3 F_Schlick(float LoH, vec3 f0) {
  float f = pow(1.0 - LoH, 5.0);
  return f + f0 * (1.0 - f);
}

struct BRDF {
  vec3 base_color;
  float metallic;
  float perceptual_roughness;
};

float min_reflectivity = 0.04;

float one_minus_reflectivity(BRDF brdf) {
  return (1.0 - min_reflectivity) * (1.0 - brdf.metallic);
}

vec3 get_reflection(BRDF brdf) {
  // similar to
  // URP(https://docs.unity3d.com/Packages/com.unity.render-pipelines.universal@10.3/manual/index.html)
  // i guess. The reflectivity actually changes for dielectric materials
  // according to their IORs(index of reflection), but most of them are around
  // 0.02-0.05.
  return mix(vec3(min_reflectivity), brdf.base_color, brdf.metallic);
}

vec3 specular_BRDF(BRDF brdf, vec3 n, vec3 v, vec3 l) {
  vec3 h = normalize(v + l);

  float NoV = abs(dot(n, v)) + 1e-5;
  float NoL = clamp(dot(n, l), 0.0, 1.0);
  float NoH = clamp(dot(n, h), 0.0, 1.0);
  float LoH = clamp(dot(l, h), 0.0, 1.0);

  vec3 f0 = get_reflection(brdf);

  float roughness = brdf.perceptual_roughness * brdf.perceptual_roughness;
  float D = D_GGX(NoH, roughness);
  vec3 F = F_Schlick(LoH, f0);
  float V = V_SmithGGXCorrelated(NoV, NoL, roughness);

  return D * V * F;
}

vec3 diffuse_BRDF(BRDF brdf) {
  return brdf.base_color * one_minus_reflectivity(brdf);
}

vec3 environment_BRDF(BRDF brdf, float NoV) {
  // similar to URP
  float perceptual_smoothness = 1.0 - brdf.perceptual_roughness;
  float reflectivity = 1.0 - one_minus_reflectivity(brdf);
  float gazing = clamp(perceptual_smoothness + reflectivity, 0.0, 1.0);
  float fresnel = pow(1.0 - NoV, 4.0);

  vec3 reflection = get_reflection(brdf);

  float roughness = brdf.perceptual_roughness * brdf.perceptual_roughness;
  return mix(reflection, vec3(gazing), fresnel) / (roughness * roughness + 1.0);
}

void main() {
  vec3 n_vs = get_normal_vs();
  vec3 l_vs = light_dir_vs;
  vec3 v_vs = -normalize(position_vs);
  n_vs = faceforward(n_vs, -v_vs, n_vs);

  vec4 base_color = get_base_color();

  BRDF brdf;
  // always premultiply alpha for simplicity
  brdf.base_color = base_color.rgb * base_color.a;
  // see
  // https://github.com/sbtron/glTF/blob/30de0b365d1566b1bbd8b9c140f9e995d3203226/specification/2.0/README.md#pbrmetallicroughnessmetallicroughnesstexture
  // for gltf metallic roughness packing rule.
  vec2 metallic_roughness = texture(metallic_roughness_tex, uv0_vs).xy;
  brdf.metallic = metallic_roughness.r * metallic_factor;
  brdf.perceptual_roughness = metallic_roughness.g * roughness_factor;

  float NoV = clamp(dot(n_vs, v_vs), 0.0, 1.0);
  float NoL = clamp(dot(n_vs, l_vs), 0.0, 1.0);

  vec3 fr = specular_BRDF(brdf, n_vs, v_vs, l_vs);
  vec3 fd = diffuse_BRDF(brdf);
  vec3 fe = environment_BRDF(brdf, NoV);

  vec3 directional = (fr + fd) * light_radiance * NoL;
  vec3 emission = get_emission();
  vec3 environment = (fd + fe) * occlude_color(env_irradiance);

  frag_color_out = vec4(directional + emission + environment, base_color.a);
}