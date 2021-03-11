#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

class Buffer {
public:
  Buffer(void *data, size_t size);
  ~Buffer();

  GLuint get() const;

private:
  GLuint _id{};
};

class VertexArray {
public:
  VertexArray();
  ~VertexArray();

  GLuint get() const;

private:
  GLuint _id{};
};

class Mesh {
public:
  struct Vertex {
    glm::vec3 position; // location 0
    glm::vec3 normal;   // location 1
    glm::vec4 tangent;  // location 2
    glm::vec2 uv0;      // location 3
    glm::vec2 uv1;      // location 4
    glm::vec4 color;    // location 5
  };

  Mesh(const Vertex *vertices,
       uint32_t vertex_count,
       const uint32_t *indices,
       uint32_t index_count);

  void draw();

private:
  uint32_t _draw_count = 0;

  std::unique_ptr<VertexArray> _vao{};
  std::unique_ptr<Buffer> _vertex_buffer{};
  std::unique_ptr<Buffer> _index_buffer{};
};