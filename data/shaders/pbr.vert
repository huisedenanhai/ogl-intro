#version 330 core

layout(location = 0) in vec3 position_os;
layout(location = 1) in vec3 normal_os;
layout(location = 2) in vec4 tangent_os;
layout(location = 3) in vec2 uv0_os;

out vec3 position_vs;
out vec3 normal_vs;
out vec3 tangent_vs;
out vec3 bitangent_vs;
out vec2 uv0_vs;

vec3 transform_normal(mat4 mat_inverse, vec3 n) {
  return vec3(dot(mat_inverse[0].xyz, n),
              dot(mat_inverse[1].xyz, n),
              dot(mat_inverse[2].xyz, n));
}

vec3 transform_vector(mat4 mat, vec3 v) {
  return mat[0].xyz * v.x + mat[1].xyz * v.y + mat[2].xyz * v.z;
}

vec4 transform_position(mat4 mat, vec3 p) {
  return mat * vec4(p, 1.0);
}

vec3 calculate_bitangent(vec3 normal, vec4 tangent) {
  return cross(normal, tangent.xyz) * tangent.w;
}

layout(std140) uniform Transform {
  mat4 MV;
  mat4 I_MV;
  mat4 P;
};

void main() {
  position_vs = transform_position(MV, position_os).xyz;
  normal_vs = normalize(transform_normal(I_MV, normal_os));
  tangent_vs = normalize(transform_vector(MV, tangent_os.xyz));
  vec3 bitangent_os = calculate_bitangent(normal_os, tangent_os);
  bitangent_vs = normalize(transform_vector(MV, bitangent_os));
  uv0_vs = uv0_os;

  gl_Position = transform_position(P, position_vs);
}