#pragma once

#include <string>
#include <string_view>
#include <GLFW/glfw3.h>

#include "logger.hpp"
#include "app_context.hpp"


class Scene {
public:
  virtual ~Scene() = default;

  virtual void Init(IAppContext* ctx) = 0;
  virtual void Update(float dt) = 0;
  virtual void Render() = 0;
  virtual void RenderInterface(int window_width, int window_height) = 0;
  virtual void Cleanup() = 0;
  virtual std::string Name() const = 0;

  virtual void OnMouseMoveEvent(float x, float y) {}
  virtual void OnMouseButtonEvent(Mouse mouse, bool pressed) {}
  virtual void OnKeyboardEvent(Key key, bool pressed) {}

protected:
  float GetTime() const {
    return static_cast<float>(glfwGetTime());
  }

  template<typename... Args>
  void LogDebug(std::string_view format, Args&&... args) {
    auto* logger = Logger::GetRootLogger();
    quill::debug(logger, format.data(), std::forward<Args>(args)...);
  }

  template<typename... Args>
  void LogInfo(std::string_view format, Args&&... args) {
    auto* logger = Logger::GetRootLogger();
    quill::info(logger, format.data(), std::forward<Args>(args)...);
  }

  template<typename... Args>
  void LogWarning(std::string_view format, Args&&... args) {
    auto* logger = Logger::GetRootLogger();
    quill::warning(logger, format.data(), std::forward<Args>(args)...);
  }

  template<typename... Args>
  void LogError(std::string_view format, Args&&... args) {
    auto* logger = Logger::GetRootLogger();
    quill::error(logger, format.data(), std::forward<Args>(args)...);
  }
};
