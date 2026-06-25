#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "types.hpp"


class Image {
public:
  unsigned int width, height, num_components;
  std::shared_ptr<void> data;

  static Image Load(std::string_view path, int req_num_components = 4, bool flip_vertically = true, bool floating_point = false);
};
