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
  static void FrameBufferSizeCallback(GLFWwindow* window, int width, int height);
  static void ProcessInput(GLFWwindow* window);

};
