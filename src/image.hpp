#pragma once

#include <string_view>
#include <vector>
#include <stb_image.h>

#include "types.hpp"
#include "logger.hpp"


class Image {
public:
  unsigned int width, height;
  std::shared_ptr<u8> data;

  static Image Load(std::string_view path) {
    const int num_channels = 4;
    int width, height;
    unsigned char* bytes = stbi_load(path.data(), &width, &height, nullptr, num_channels);
    if (!bytes) {
      auto* logger = Logger::GetRootLogger();
      quill::error(logger, "[IMAGE] Failed to load image: {}", stbi_failure_reason());
      return {};
    }

    return Image{
      .width = static_cast<unsigned int>(width),
      .height = static_cast<unsigned int>(height),
      .data = { bytes, stbi_image_free },
    };
  }
};
