#pragma once

#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

class Data {
public:
  static fs::path data_path();
  static std::vector<uint8_t> load(const fs::path &name);
  static fs::path resolve(const fs::path& name);
};