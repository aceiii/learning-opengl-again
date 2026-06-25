#pragma once

#include <string>
#include <string_view>


struct TextureOptions {
  int wrap_s;
  int wrap_t;
  int min_filter;
  int mag_filter;
  bool linear;
  bool hdr;
  bool flip_vertically = true;
};

struct Texture {
  unsigned int id = 0;
  std::string type;
  std::string path;

  static Texture Load(std::string_view type, std::string_view path, TextureOptions options = {});
  void Unload();
};
