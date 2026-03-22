#pragma once

#include <functional>
#include <string_view>
#include <quill/Logger.h>
#include <quill/LogFunctions.h>


namespace Logger {
  using LogCallback = std::function<void(std::string_view)>;

  quill::Logger* GetRootLogger();
  uint32_t AddLogCallback(LogCallback callback);
  void RemoveLogCallback(uint32_t id);
  void ClearCallbacks();
}
