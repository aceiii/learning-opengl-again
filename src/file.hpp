#pragma once

#include <fstream>
#include <sstream>
#include <string>

#include "logger.hpp"


namespace File {

  std::string ReadContents(const std::string& path) {
    std::ifstream file(path, std::ios::in);
    if (file.fail() || file.bad()) {
      auto* logger = Logger::GetRootLogger();
      quill::error(logger, "[FILE] Failed to open file: {}", strerror(errno));
      return {};
    }

    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
  }
}
