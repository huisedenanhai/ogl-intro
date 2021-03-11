#pragma once

#include <glm/glm.hpp>
#include <queue>

glm::vec3 polar_to_cartesian(float yaw, float pitch);

template <typename T> class FixSizeQueue {
public:
  FixSizeQueue(size_t max_size) : _max_size(max_size) {}

  void push(const T &value) {
    _queue.push(value);
    constrain_size();
  }

  template <typename... Args> void emplace(Args &&...args) {
    _queue.emplace(std::forward<Args>(args)...);
    constrain_size();
  }

  const T &front() const {
    return _queue.front();
  }

  const T &back() const {
    return _queue.back();
  }

  bool empty() const {
    return _queue.empty();
  }

  size_t size() const {
    return _queue.size();
  }

private:
  void constrain_size() {
    while (_queue.size() > _max_size) {
      _queue.pop();
    }
  }

  size_t _max_size;
  std::queue<T> _queue{};
};