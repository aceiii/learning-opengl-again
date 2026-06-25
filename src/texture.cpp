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
  auto logger = Logger::GetRootLogger();
  std::string filename{path};

  unsigned int texture_id;
  glGenTextures(1, &texture_id);

  Image image = Image::Load(filename, 0, options.flip_vertically, options.hdr);

  if (!image.data) {
    quill::warning(logger, "Texture failed to load at path: '{}'", filename);
    return {
      .id = texture_id,
      .type = std::string{type},
      .path = std::string{path},
    };
  }

  GLenum format, internal_format;
  GLenum byte_type = options.hdr ? GL_FLOAT : GL_UNSIGNED_BYTE;
  switch (image.num_components) {
    case 1:
      internal_format = options.hdr ? GL_R32F : GL_RED;
      format = GL_RED;
      byte_type = GL_FLOAT;
      break;
    case 3:
      format = GL_RGB;
      internal_format = options.hdr ? GL_RGB16F : options.linear ? GL_SRGB : GL_RGB;
      break;
    case 4:
      format = GL_RGBA;
      internal_format = options.hdr ? GL_RGBA16F : options.linear ? GL_SRGB_ALPHA : GL_RGBA;
      break;
    default:
      quill::warning(logger, "Unknown texture format: {}", image.num_components);
  }

  quill::info(logger, "Loading texture:'{}', format:{}, internal_format:{}, byte_type:{}", path, format, internal_format, byte_type);

  glBindTexture(GL_TEXTURE_2D, texture_id);
  glTexImage2D(GL_TEXTURE_2D, 0, internal_format, image.width, image.height, 0, format, byte_type, image.data.get());
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

void Texture::Unload() {
  if (!id) {
    return;
  }

  glDeleteTextures(1, &id);
  id = 0;
}
