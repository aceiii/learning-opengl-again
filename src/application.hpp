#pragma once

#include <expected>
#include <string>
#include <memory>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "applog.hpp"
#include "scene.hpp"
#include "scenes/empty_scene.hpp"


class Application {
public:
  std::expected<void, std::string> Init();
  void Run();
  void Cleanup();

private:
  void Update(float dt);
  void Render();
  void RenderInterface();

  void LogCallback(std::string_view message);

  static void ErrorCallback(int error_code, const char* description);
  static void FrameBufferSizeCallback(GLFWwindow* window, int width, int height);
  static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
  static void ProcessInput(GLFWwindow* window);

  AppLog app_log_;
  std::vector<std::shared_ptr<Scene>> scenes_;
  std::shared_ptr<Scene> selected_scene_ = EmptyScene::Get();
};
