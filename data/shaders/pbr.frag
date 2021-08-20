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
  vec3 env_radiance;
};

uniform sampler2D base_color_tex;
uniform sampler2D metallic_roughness_tex;
uniform sampler2D normal_tex;
uniform sampler2D occlusion_tex;
uniform sampler2D emission_tex;
uniform sampler2D lut_tex;

layout(location = 0) out vec4 frag_color_out;

vec3 srgb_to_linear(vec3 srgb) {
  return pow(srgb, vec3(2.2));
}

vec4 get_base_color() {
  vec4 raw = texture(base_color_tex, uv0_vs);
  return vec4(srgb_to_linear(raw.rgb), raw.a) * base_color_factor;
}

vec3 safe_normalize(vec3 v) {
  return dot(v, v) == 0 ? v : normalize(v);
}

vec3 decode_normal_ts() {
  vec3 normal = texture(normal_tex, uv0_vs).xyz * 2.0 - 1.0;
  return safe_normalize(normal * vec3(normal_scale, normal_scale, 1.0));
}

vec3 get_normal_vs() {
  vec3 normal_ts = decode_normal_ts();
  // avoid NAN when tangent is not present
  return normal_ts.x * safe_normalize(tangent_vs) +
         normal_ts.y * safe_normalize(bitangent_vs) +
         normal_ts.z * safe_normalize(normal_vs);
}

vec3 occlude_color(vec3 unocclude_color) {
  float occlusion = texture(occlusion_tex, uv0_vs).r;
  return mix(unocclude_color, unocclude_color * occlusion, occlusion_strength);
}

vec3 get_emission() {
  return srgb_to_linear(texture(emission_tex, uv0_vs).xyz) * emission_factor;
}

float PI = 3.14159;

float square(float v) {
  return v * v;
}

// there is a ton of resouces for the theory of PBR. their basic principles are
// similar but details may vary, which can be confusing for beginners on this
// topic, especially when there are typos in the expression.
//
// we follow the paper
// 'Microfacet Models for Refraction through Rough Surfaces'
// which introduce the GGX distribution to model the scattering of rough glass.
//
// GGX is wildly used in modern game engines.
//
// we use SI(International System of Units) for physical terms.
float D_GGX(float NoH, float roughness) {
  float a2 = square(roughness);
  float c2 = square(NoH);
  return a2 / (PI * square(a2 * c2 + 1.0 - c2));
}

float V1_SmithGGX(float NoV, float roughness) {
  float a2 = square(roughness);
  return 1.0 / (NoV + sqrt(square(NoV) + a2 * (1.0 - square(NoV))));
}

float V_SmithGGX(float NoV, float NoL, float roughness) {
  return V1_SmithGGX(NoV, roughness) * V1_SmithGGX(NoL, roughness);
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

  float NoV = clamp(abs(dot(n, v)), 0.0, 1.0);
  float NoL = clamp(dot(n, l), 0.0, 1.0);
  float NoH = clamp(dot(n, h), 0.0, 1.0);
  float LoH = clamp(dot(l, h), 0.0, 1.0);

  vec3 f0 = get_reflection(brdf);

  float roughness = brdf.perceptual_roughness * brdf.perceptual_roughness;
  float D = D_GGX(NoH, roughness);
  vec3 F = F_Schlick(LoH, f0);
  float V = V_SmithGGX(NoV, NoL, roughness);

  // remove NAN
  return max(D * V * F, vec3(0.0));
}

vec3 diffuse_BRDF(BRDF brdf) {
  return brdf.base_color * one_minus_reflectivity(brdf) / PI;
}

vec3 environment_BRDF(BRDF brdf, float NoV) {
  // sample pre integrated BRDF lut
  float roughness = brdf.perceptual_roughness * brdf.perceptual_roughness;
  vec2 env_brdf_factor = texture(lut_tex, vec2(roughness, NoV)).xy;
  vec3 f0 = get_reflection(brdf);
  return f0 * env_brdf_factor.x + env_brdf_factor.y;
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
  // https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#reference-pbrmetallicroughness
  // for gltf metallic roughness packing rule.
  vec4 metallic_roughness = texture(metallic_roughness_tex, uv0_vs);
  brdf.metallic = metallic_roughness.b * metallic_factor;
  brdf.perceptual_roughness = metallic_roughness.g * roughness_factor;

  float NoV = clamp(dot(n_vs, v_vs), 0.0, 1.0);
  float NoL = clamp(dot(n_vs, l_vs), 0.0, 1.0);

  vec3 fr = specular_BRDF(brdf, n_vs, v_vs, l_vs);
  vec3 fd = diffuse_BRDF(brdf);
  vec3 fe = environment_BRDF(brdf, NoV);

  vec3 directional = (fr + fd) * light_radiance * NoL;
  vec3 emission = get_emission();

  vec3 env_irradiance = occlude_color(env_radiance * PI);
  vec3 environment = (fd + fe) * env_irradiance;

  frag_color_out = vec4(directional + emission + environment, base_color.a);
}