#include <glad/glad.h>

#include "image.hpp"
#include "texture.hpp"
#include "logger.hpp"


Texture Texture::Load(std::string_view type, std::string_view path) {
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

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glBindTexture(GL_TEXTURE_2D, 0);

  return {
    .id = texture_id,
    .type = std::string{type},
    .path = std::string{path},
  };
}
