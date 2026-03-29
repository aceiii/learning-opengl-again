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


class Application final : public IAppContext {
public:
  ~Application() = default;

  std::expected<void, std::string> Init();
  void Run();
  void Cleanup();

public:
  float GetFrameTime() const override;
  std::pair<int, int> GetWindowSize() const override;
  virtual std::pair<float, float> GetMousePosition() const override;
  bool IsKeyDown(Key key) const override;
  bool IsMouseButtonDown(Mouse mouse) const override;

  void RequestQuit() override;
  void ToggleUI(bool enabled) override;
  void CaptureMouse(bool enabled) override;
  void SetMousePosition(float x, float y) override;

private:
  void Update(float dt);
  void Render();
  void RenderInterface();
  void ProcessInput(GLFWwindow* window, float dt);
  void LogCallback(std::string_view message);

  static Key FromGlfwKey(int key);
  static int ToGlfwKey(Key key);

  static Mouse FromGlfwMouse(int mouse);
  static int ToGlfwMouse(Mouse mouse);

  static void ErrorCallback(int error_code, const char* description);
  static void FrameBufferSizeCallback(GLFWwindow* window, int width, int height);
  static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
  static void MouseCursorCallback(GLFWwindow* window, double x_pos, double y_pos);
  static void KeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
  static void ScrollCallback(GLFWwindow* window, double x_offset, double y_offset);

  AppLog app_log_;
  std::vector<std::shared_ptr<Scene>> scenes_;

  bool enable_interface_ = true;
  float delta_time_ = 0;
  int window_width_ = 0;
  int window_height_ = 0;
};
