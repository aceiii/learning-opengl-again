#pragma once

#include <expected>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>


class Application {
public:
  std::expected<void, std::string> Init();
  void Run();
  void Cleanup();

private:
  void Update();
  void Render();

  static void FrameBufferSizeCallback(GLFWwindow* window, int width, int height);
  static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
  static void ProcessInput(GLFWwindow* window);
};
