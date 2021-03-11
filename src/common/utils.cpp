#include "utils.hpp"

glm::vec3 polar_to_cartesian(float yaw, float pitch) {
  float y = cosf(pitch);
  float sinPitch = sinf(pitch);
  float x = sinPitch * cosf(yaw);
  float z = sinPitch * sinf(yaw);
  return glm::vec3(x, y, z);
}
