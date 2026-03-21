#pragma once

#include <imgui.h>
#include <GLFW/glfw3.h>

namespace ImGuiImpl {
  void Init(GLFWwindow* window);
  void NewFrame();
  void RenderDrawData();
}
