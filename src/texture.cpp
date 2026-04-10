#include <glad/glad.h>

#include "image.hpp"
#include "texture.hpp"
#include "logger.hpp"

namespace {
  constexpr int kDefaultWrapS = GL_REPEAT;
  constexpr int kDefaultWrapT = GL_REPEAT;
  constexpr int kDefaultMinFilter = GL_LINEAR_MIPMAP_LINEAR;
  constexpr int kDefaultMagFilter = GL_LINEAR;
}

Texture Texture::Load(std::string_view type, std::string_view path, TextureOptions options) {
  std::string filename{path};

  unsigned int texture_id;
  glGenTextures(1, &texture_id);

  Image image = Image::Load(filename, 0);

  if (!image.data) {
    auto logger = Logger::GetRootLogger();
    quill::warning(logger, "Texture failed to load at path: '{}'", filename);
    return {
      .id = texture_id,
      .type = std::string{type},
      .path = std::string{path},
    };
  }

  GLenum format;
  switch (image.num_components) {
    case 1: format = GL_RED; break;
    case 3: format = GL_RGB; break;
    case 4: format = GL_RGBA; break;
    default: {
      auto logger = Logger::GetRootLogger();
      quill::warning(logger, "Unknown texture format: {}", image.num_components);
    }
  }

  glBindTexture(GL_TEXTURE_2D, texture_id);
  glTexImage2D(GL_TEXTURE_2D, 0, format, image.width, image.height, 0, format, GL_UNSIGNED_BYTE, image.data.get());
  glGenerateMipmap(GL_TEXTURE_2D);

  auto wrap_s = options.wrap_s ? options.wrap_s : kDefaultWrapS;
  auto wrap_t = options.wrap_t ? options.wrap_t : kDefaultWrapT;
  auto min_filter = options.min_filter ? options.min_filter : kDefaultMinFilter;
  auto mag_filter = options.mag_filter ? options.mag_filter : kDefaultMagFilter;

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);

  glBindTexture(GL_TEXTURE_2D, 0);

  return {
    .id = texture_id,
    .type = std::string{type},
    .path = std::string{path},
  };
}
