#pragma once

#include <expected>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "applog.hpp"


class Application {
public:
  std::expected<void, std::string> Init();
  void Run();
  void Cleanup();

private:
  void Update();
  void Render();
  void RenderInterface();

  void LogCallback(std::string_view message);

  static void ErrorCallback(int error_code, const char* description);
  static void FrameBufferSizeCallback(GLFWwindow* window, int width, int height);
  static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
  static void ProcessInput(GLFWwindow* window);

  AppLog app_log_;
};
