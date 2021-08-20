#version 330 core

in vec2 uv_fs;

layout(location = 0) out vec4 frag_color;

float PI = 3.14159;

float square(float v) {
  return v * v;
}

// more detailed comments on BRDF can be found in pbr.frag
float V1_SmithGGX(float NoV, float roughness) {
  float a2 = square(roughness);
  return 1.0 / (NoV + sqrt(square(NoV) + a2 * (1.0 - square(NoV))));
}

float V_SmithGGX(float NoV, float NoL, float roughness) {
  return V1_SmithGGX(NoV, roughness) * V1_SmithGGX(NoL, roughness);
}

// Low-discrepancy sequence for quasi monte carlo intergation
float halton(float i, float base) {
  float v = 0.0;
  float f = 1.0;
  while (i > 0) {
    f /= base;
    v += f * mod(i, base);
    i = floor(i / base);
  }
  return v;
}

vec2 hammersly(float i, float count) {
  return vec2(i / count, halton(i, 2.0));
}

// importance sampling D(h) (n.h)
//
// most of noise comes from GGX sampling
// importance sampling other terms (like cosine sampling hemisphere) does not
// help much
//
// u is a uniform random variable in [0, 1)
float sample_NoH(float roughness, float u) {
  // v in range (0, 1]
  float v = 1.0 - u;
  // the denorm will not be 0.0
  return sqrt(v / (v + square(roughness) * (1.0 - v)));
}

vec3 sample_h(float roughness, vec2 u) {
  float NoH = sample_NoH(roughness, u.x);
  float r = sqrt(max(1.0 - square(NoH), 0.0));
  float phi = 2.0 * PI * u.y;
  float y = NoH;
  float x = cos(phi) * r;
  float z = sin(phi) * r;
  return vec3(x, y, z);
}

float calculate_dldh(vec3 v, vec3 l, vec3 h) {
  return square(length(l + v)) / dot(l, h);
}

vec3 specular_BRDF_factor(float roughness, vec3 n, vec3 v, vec3 h) {
  vec3 l = normalize(2.0 * dot(h, v) * h - v);

  if (dot(n, l) < 0.0) {
    return vec3(0.0);
  }

  float NoV = clamp(abs(dot(n, v)), 0.0, 1.0);
  float NoL = clamp(dot(n, l), 0.0, 1.0);
  float NoH = clamp(dot(n, h), 0.0, 1.0);
  float LoH = clamp(dot(l, h), 0.0, 1.0);

  // the Schlick approximation
  // F_Schlick = f + f0 * (1 - f)
  // f0 is calculated based on the base color and the metallic value
  // we only pre-integrate the term f, thus the pre-computed result depends only
  // on roughness and NoV
  // the contribution of f0 can be added back in the actual env brdf calculation
  float F = pow(1.0 - LoH, 5.0);
  float V = V_SmithGGX(NoV, NoL, roughness);

  // the actual pdf is D (n.h)
  // but D is canceled out
  float dldh = calculate_dldh(v, l, h);
  float pdf = NoH;

  vec3 result = vec3(1 - F, F, 0.0) * V * NoL;
  result *= dldh / pdf;

  // remove NAN
  return max(result, vec3(0.0));
}

void main() {
  float roughness = uv_fs.x;
  float NoV = uv_fs.y;

  vec3 n = vec3(0.0, 1.0, 0.0);
  vec3 v = vec3(sqrt(max(0.0, 1.0 - NoV * NoV)), NoV, 0.0);

  int sample_count = 1024;

  vec3 env_brdf = vec3(0.0);
  int acc_sample_count = 0;
  for (int i = 0; i < sample_count; i++) {
    vec2 u = hammersly(i, sample_count);
    vec3 h = sample_h(roughness, u);
    env_brdf += specular_BRDF_factor(roughness, n, v, h);
  }

  env_brdf /= float(sample_count);
  // the integeration assumes the radiance is 1.
  // but when the term used for actual shading, we want to
  // multiply it with irradiance.
  // the irradiance of uniform radiance is PI.
  // scale the env brdf to make the magnitude correct.
  env_brdf /= PI;
  frag_color = vec4(env_brdf, 1.0);
}