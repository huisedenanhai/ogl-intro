#include "data.hpp"
#include <config.h>
#include <fstream>

fs::path Data::data_path() {
  return DATA_PATH;
}

std::vector<uint8_t> Data::load(const fs::path &name) {
  auto full_path = resolve(name);
  std::ifstream ifs(full_path);
  return {std::istreambuf_iterator<char>(ifs), {}};
}

fs::path Data::resolve(const fs::path &name) {
  return data_path() / name;
}