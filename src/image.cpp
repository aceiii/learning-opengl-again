#include <string_view>
#include <vector>
#include <stb_image.h>

#include "image.hpp"
#include "logger.hpp"


Image Image::Load(std::string_view path, int req_num_components, bool flip_vertically, bool floating_point) {
  int width, height, num_components;

  stbi_set_flip_vertically_on_load(flip_vertically);

  if (floating_point) {
    float* bytes = stbi_loadf(path.data(), &width, &height, &num_components, req_num_components);
    if (!bytes) {
      auto* logger = Logger::GetRootLogger();
      quill::error(logger, "[IMAGE] Failed to load image: {}", stbi_failure_reason());
      return {};
    }

    return Image{
      .width = static_cast<unsigned int>(width),
      .height = static_cast<unsigned int>(height),
      .num_components = static_cast<unsigned int>(num_components),
      .data = { bytes, stbi_image_free },
    };
  } else {
    unsigned char* bytes = stbi_load(path.data(), &width, &height, &num_components, req_num_components);
    if (!bytes) {
      auto* logger = Logger::GetRootLogger();
      quill::error(logger, "[IMAGE] Failed to load image: {}", stbi_failure_reason());
      return {};
    }

    return Image{
      .width = static_cast<unsigned int>(width),
      .height = static_cast<unsigned int>(height),
      .num_components = static_cast<unsigned int>(num_components),
      .data = { bytes, stbi_image_free },
    };
  }
}
