#pragma once

#include <string>
#include <string_view>


struct Texture {
  unsigned int id;
  std::string type;
  std::string path;

  static Texture Load(std::string_view type, std::string_view path);
};
