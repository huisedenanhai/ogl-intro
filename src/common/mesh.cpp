#include "mesh.hpp"

Buffer::Buffer(void *data, size_t size) {
  glGenBuffers(1, &_id);
  glBindBuffer(GL_ARRAY_BUFFER, _id);
  glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

Buffer::~Buffer() {
  glDeleteBuffers(1, &_id);
}

GLuint Buffer::get() const {
  return _id;
}

VertexArray::VertexArray() {
  glGenVertexArrays(1, &_id);
}

VertexArray::~VertexArray() {
  glDeleteVertexArrays(1, &_id);
}

GLuint VertexArray::get() const {
  return _id;
}

Mesh::Mesh(const Vertex *vertices,
           uint32_t vertex_count,
           const uint32_t *indices,
           uint32_t index_count) {
  _vao = std::make_unique<VertexArray>();
  if (vertices == nullptr) {
    return;
  }
  _vertex_buffer =
      std::make_unique<Buffer>((void *)vertices, sizeof(Vertex) * vertex_count);
  _draw_count = vertex_count;
  if (indices != nullptr) {
    _index_buffer = std::make_unique<Buffer>((void *)indices,
                                             sizeof(uint32_t) * index_count);
    _draw_count = index_count;
  } else {
    _index_buffer = nullptr;
  }

  glBindVertexArray(_vao->get());
  glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer->get());
  if (indices != nullptr) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _index_buffer->get());
  }

#define ENABLE_LOCATION(location, count, field)                                \
  glVertexAttribPointer(location,                                              \
                        count,                                                 \
                        GL_FLOAT,                                              \
                        GL_FALSE,                                              \
                        sizeof(Vertex),                                        \
                        (void *)offsetof(Vertex, field));                      \
  glEnableVertexAttribArray(location)

  ENABLE_LOCATION(0, 3, position);
  ENABLE_LOCATION(1, 3, normal);
  ENABLE_LOCATION(2, 4, tangent);
  ENABLE_LOCATION(3, 2, uv0);
  ENABLE_LOCATION(4, 2, uv1);
  ENABLE_LOCATION(5, 4, color);

#undef ENABLE_LOCATION
}

void Mesh::draw() {
  if (_draw_count == 0) {
    return;
  }
  glBindVertexArray(_vao->get());
  if (_index_buffer != nullptr) {
    glDrawElements(
        GL_TRIANGLES, (GLsizei)_draw_count, GL_UNSIGNED_INT, nullptr);
  } else {
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)_draw_count);
  }
}
