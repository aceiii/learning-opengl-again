#include <array>
#include <print>
#include <string>
#include <string_view>
#include <quill/SimpleSetup.h>
#include <quill/LogFunctions.h>
#include "application.hpp"
#include "imgui_impl.hpp"
#include "logger.hpp"
#include "scenes.hpp"


namespace {
  quill::Logger* logger = nullptr;
  GLFWwindow* g_window = nullptr;
  std::shared_ptr<Scene> g_selected_scene_ = EmptyScene::Get();
}


std::expected<void, std::string> Application::Init() {
  logger = Logger::GetRootLogger();
  Logger::AddLogCallback(std::bind(&Application::LogCallback, this, std::placeholders::_1));

  quill::info(logger, "[APPLICATION] Initializing application");

  glfwSetErrorCallback(ErrorCallback);

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  g_window = glfwCreateWindow(800, 600, "Learning OpenGL AGAIN!", nullptr, nullptr);
  if (g_window == nullptr) {
    return std::unexpected{"Failed to create GLFW Window"};
  }

  glfwMakeContextCurrent(g_window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    return std::unexpected{"Failed to initialize GLAD"};
  }

  if (glfwRawMouseMotionSupported()) {
    quill::info(logger, "[APPLICATION] Enabling raw mouse motion");
    glfwSetInputMode(g_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
  }

  glfwSetFramebufferSizeCallback(g_window, FrameBufferSizeCallback);
  glfwSetMouseButtonCallback(g_window, MouseButtonCallback);
  glfwSetCursorPosCallback(g_window, MouseCursorCallback);
  glfwSetScrollCallback(g_window, ScrollCallback);
  glfwSetKeyCallback(g_window, KeyboardCallback);

  ImGuiImpl::Init(g_window);

  int framebuffer_width, framebuffer_height;
  glfwGetFramebufferSize(g_window, &framebuffer_width, &framebuffer_height);
  glViewport(0, 0, framebuffer_width, framebuffer_height);

  scenes_.push_back(EmptyScene::Get());
  scenes_.push_back(std::make_shared<HelloTriangle>());
  scenes_.push_back(std::make_shared<ShadersScene>());
  scenes_.push_back(std::make_shared<TexturesScene>());
  scenes_.push_back(std::make_shared<TransformationsScene>());
  scenes_.push_back(std::make_shared<CoordinateSystemsScene>());
  scenes_.push_back(std::make_shared<CameraScene>());
  scenes_.push_back(std::make_shared<BasicLightingScene>());
  scenes_.push_back(std::make_shared<MaterialsScene>());
  scenes_.push_back(std::make_shared<LightingMapsScene>());
  scenes_.push_back(std::make_shared<LightCastersScene>());
  scenes_.push_back(std::make_shared<MultipleLightsScene>());
  scenes_.push_back(std::make_shared<ModelScene>());
  scenes_.push_back(std::make_shared<DepthTestScene>());
  scenes_.push_back(std::make_shared<StencilTestScene>());
  scenes_.push_back(std::make_shared<BlendingScene>());
  scenes_.push_back(std::make_shared<CullingScene>());
  scenes_.push_back(std::make_shared<FramebuffersScene>());

  return {};
}

void Application::Run() {
  double last_frame_time = glfwGetTime();

  while (!glfwWindowShouldClose(g_window)) {
    double current_time = glfwGetTime();
    delta_time_ = current_time - last_frame_time;
    last_frame_time = current_time;

    glfwGetWindowSize(g_window, &window_width_, &window_height_);

    Update(delta_time_);
    Render();
    RenderInterface();

    ImGuiImpl::RenderDrawData();
    glfwSwapBuffers(g_window);
    glfwPollEvents();
  }
}

void Application::Cleanup() {
  quill::info(logger, "[APPLICATION] Cleaning up application");

  for (auto& scene : scenes_) {
    scene->Cleanup();
  }
  scenes_.clear();

  glfwTerminate();

  Logger::ClearCallbacks();
}

float Application::GetTime() const {
  return glfwGetTime();
}

float Application::GetFrameTime() const {
  return delta_time_;
}

std::pair<int, int> Application::GetWindowSize() const {
  return std::make_pair(window_width_, window_height_);
}

std::pair<float, float> Application::GetMousePosition() const {
  double_t x, y;
  glfwGetCursorPos(g_window, &x, &y);
  return std::make_pair(static_cast<float>(x), static_cast<float>(y));
}

bool Application::IsKeyDown(Key key) const {
  ImGuiIO& io = ImGui::GetIO();
  return !io.WantCaptureKeyboard && glfwGetKey(g_window, ToGlfwKey(key)) == GLFW_PRESS;
}

bool Application::IsMouseButtonDown(Mouse mouse) const {
  ImGuiIO& io = ImGui::GetIO();
  return !io.WantCaptureMouse && glfwGetMouseButton(g_window, ToGlfwMouse(mouse)) == GLFW_PRESS;
}

void Application::RequestQuit() {
  glfwSetWindowShouldClose(g_window, true);
}

void Application::ToggleUI(bool enabled) {
  enable_interface_ = enabled;
}

void Application::CaptureMouse(bool enabled) {
  glfwSetInputMode(g_window, GLFW_CURSOR, enabled ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

void Application::SetMousePosition(float x, float y) {
  glfwSetCursorPos(g_window, x, y);
}

glm::vec3 Application::GetBackgroundColor() const {
  return bgcolor_;
}

void Application::SetBackgroundColor(const glm::vec3& color) {
  bgcolor_ = color;
}

void Application::Update(float dt) {
  if (g_selected_scene_) {
    g_selected_scene_->Update(dt);
  }
}

void Application::Render() {
  glClearColor(bgcolor_.r, bgcolor_.g, bgcolor_.b, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  if (g_selected_scene_) {
    g_selected_scene_->Render();
  }
}

void Application::RenderInterface() {
  static bool show_logs = true;
  static bool show_demo_window = false;

  ImGuiImpl::NewFrame();
  if (enable_interface_) {
    ImGui::BeginMainMenuBar();
    {
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Quit", "Ctrl+Q")) {
          glfwSetWindowShouldClose(g_window, true);
        }
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("Demo Window", nullptr, &show_demo_window);
        ImGui::MenuItem("Logs", nullptr, &show_logs);
        ImGui::EndMenu();
      }
    }
    ImGui::EndMainMenuBar();

    if (show_demo_window) {
      ImGui::ShowDemoWindow(&show_demo_window);
    }

    if (show_logs) {
      constexpr auto padding = 5.0f;
      ImGui::SetNextWindowPos(ImVec2(padding, window_height_ - padding), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
      ImGui::SetNextWindowSize(ImVec2(window_width_ - padding - padding, 180.0f - padding - padding), ImGuiCond_Always);
      if (ImGui::Begin("Logs", &show_logs, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking)) {
        app_log_.Draw();
      }
      ImGui::End();
    }

    {
      constexpr auto menu_bar_height = 24.0f;
      constexpr auto padding = 5.0f;
      ImGui::SetNextWindowPos(ImVec2(padding, menu_bar_height + padding), ImGuiCond_Always, ImVec2(0.0f, 0.0f));
      ImGui::SetNextWindowSize(ImVec2(220.0f, 0.0f), ImGuiCond_Always);
      if (ImGui::Begin("Scene", &show_logs, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration)) {
        if (ImGui::BeginCombo("Scene", g_selected_scene_->Name().c_str())) {
          for (const auto& scene : scenes_) {
            const bool is_selected = scene == g_selected_scene_;
            if (ImGui::Selectable(scene->Name().c_str(), is_selected)) {
              quill::info(logger, "[APPLICATION] Selected scene: {}", scene->Name());

              if (!is_selected) {
                g_selected_scene_->Cleanup();
                g_selected_scene_ = scene;
                g_selected_scene_->Init(this);
              }
            }
          }
          ImGui::EndCombo();
        }
      }
      ImGui::End();
    }

    if (g_selected_scene_) {
      g_selected_scene_->RenderInterface(window_width_, window_height_);
    }
  }
  ImGui::Render();
}

void Application::LogCallback(std::string_view message) {
  app_log_.AddLog(message);
}

Key Application::FromGlfwKey(int key) {
  switch (key) {
  case GLFW_KEY_ESCAPE: return Key::kKeyEscape; break;
  case GLFW_KEY_UP: return Key::kKeyUp; break;
  case GLFW_KEY_DOWN: return Key::kKeyDown; break;
  case GLFW_KEY_LEFT: return Key::kKeyLeft; break;
  case GLFW_KEY_RIGHT: return Key::kKeyRight; break;
  case GLFW_KEY_1: return Key::kKey1; break;
  case GLFW_KEY_2: return Key::kKey2; break;
  case GLFW_KEY_3: return Key::KKey3; break;
  case GLFW_KEY_4: return Key::kKey4; break;
  case GLFW_KEY_5: return Key::kKey5; break;
  case GLFW_KEY_6: return Key::kKey6; break;
  case GLFW_KEY_7: return Key::kKey7; break;
  case GLFW_KEY_8: return Key::kKey8; break;
  case GLFW_KEY_9: return Key::kKey9; break;
  case GLFW_KEY_0: return Key::kKey0; break;
  case GLFW_KEY_A: return Key::kKeyA; break;
  case GLFW_KEY_B: return Key::kKeyB; break;
  case GLFW_KEY_C: return Key::kKeyC; break;
  case GLFW_KEY_D: return Key::kKeyD; break;
  case GLFW_KEY_E: return Key::kKeyE; break;
  case GLFW_KEY_F: return Key::kKeyF; break;
  case GLFW_KEY_G: return Key::kKeyG; break;
  case GLFW_KEY_H: return Key::kKeyH; break;
  case GLFW_KEY_I: return Key::kKeyI; break;
  case GLFW_KEY_J: return Key::kKeyJ; break;
  case GLFW_KEY_K: return Key::kKeyK; break;
  case GLFW_KEY_L: return Key::kKeyL; break;
  case GLFW_KEY_M: return Key::kKeyM; break;
  case GLFW_KEY_N: return Key::kKeyN; break;
  case GLFW_KEY_O: return Key::kKeyO; break;
  case GLFW_KEY_P: return Key::kKeyP; break;
  case GLFW_KEY_Q: return Key::kKeyQ; break;
  case GLFW_KEY_R: return Key::kKeyR; break;
  case GLFW_KEY_S: return Key::kKeyS; break;
  case GLFW_KEY_T: return Key::kKeyT; break;
  case GLFW_KEY_U: return Key::kKeyU; break;
  case GLFW_KEY_V: return Key::kKeyV; break;
  case GLFW_KEY_W: return Key::kKeyW; break;
  case GLFW_KEY_X: return Key::kKeyX; break;
  case GLFW_KEY_Y: return Key::kKeyY; break;
  case GLFW_KEY_Z: return Key::kKeyZ; break;
  default: return Key::kKeyUnknown;
  }
}

int Application::ToGlfwKey(Key key) {
  switch (key) {
  case Key::kKeyEscape: return GLFW_KEY_ESCAPE; break;
  case Key::kKeyUp: return GLFW_KEY_UP; break;
  case Key::kKeyDown: return GLFW_KEY_DOWN; break;
  case Key::kKeyLeft: return GLFW_KEY_LEFT; break;
  case Key::kKeyRight: return GLFW_KEY_RIGHT; break;
  case Key::kKey1: return GLFW_KEY_1; break;
  case Key::kKey2: return GLFW_KEY_2; break;
  case Key::KKey3: return GLFW_KEY_3; break;
  case Key::kKey4: return GLFW_KEY_4; break;
  case Key::kKey5: return GLFW_KEY_5; break;
  case Key::kKey6: return GLFW_KEY_6; break;
  case Key::kKey7: return GLFW_KEY_7; break;
  case Key::kKey8: return GLFW_KEY_8; break;
  case Key::kKey9: return GLFW_KEY_9; break;
  case Key::kKey0: return GLFW_KEY_0; break;
  case Key::kKeyA: return GLFW_KEY_A; break;
  case Key::kKeyB: return GLFW_KEY_B; break;
  case Key::kKeyC: return GLFW_KEY_C; break;
  case Key::kKeyD: return GLFW_KEY_D; break;
  case Key::kKeyE: return GLFW_KEY_E; break;
  case Key::kKeyF: return GLFW_KEY_F; break;
  case Key::kKeyG: return GLFW_KEY_G; break;
  case Key::kKeyH: return GLFW_KEY_H; break;
  case Key::kKeyI: return GLFW_KEY_I; break;
  case Key::kKeyJ: return GLFW_KEY_J; break;
  case Key::kKeyK: return GLFW_KEY_K; break;
  case Key::kKeyL: return GLFW_KEY_L; break;
  case Key::kKeyM: return GLFW_KEY_M; break;
  case Key::kKeyN: return GLFW_KEY_N; break;
  case Key::kKeyO: return GLFW_KEY_O; break;
  case Key::kKeyP: return GLFW_KEY_P; break;
  case Key::kKeyQ: return GLFW_KEY_Q; break;
  case Key::kKeyR: return GLFW_KEY_R; break;
  case Key::kKeyS: return GLFW_KEY_S; break;
  case Key::kKeyT: return GLFW_KEY_T; break;
  case Key::kKeyU: return GLFW_KEY_U; break;
  case Key::kKeyV: return GLFW_KEY_V; break;
  case Key::kKeyW: return GLFW_KEY_W; break;
  case Key::kKeyX: return GLFW_KEY_X; break;
  case Key::kKeyY: return GLFW_KEY_Y; break;
  case Key::kKeyZ: return GLFW_KEY_Z; break;
  default: return 0;
  }
}

Mouse Application::FromGlfwMouse(int mouse) {
  switch (mouse) {
  case GLFW_MOUSE_BUTTON_LEFT: return Mouse::kMouseLeft;
  case GLFW_MOUSE_BUTTON_RIGHT: return Mouse::kMouseRight;
  case GLFW_MOUSE_BUTTON_MIDDLE: return Mouse::kMouseMiddle;
  }
  std::unreachable();
}

int Application::ToGlfwMouse(Mouse mouse) {
  switch (mouse) {
  case Mouse::kMouseLeft: return GLFW_MOUSE_BUTTON_LEFT;
  case Mouse::kMouseRight: return GLFW_MOUSE_BUTTON_RIGHT;
  case Mouse::kMouseMiddle: return GLFW_MOUSE_BUTTON_MIDDLE;
  }
  std::unreachable();
}

void Application::ErrorCallback(int error_code, const char* description) {
  quill::error(logger, "[GLFW] code={}, description={}", error_code, description);
}

void Application::FrameBufferSizeCallback(GLFWwindow* window, int width, int height) {
  quill::info(logger, "[GLFW] Frame buffer resize width={}, height={}", width, height);
  glViewport(0, 0, width, height);
}

void Application::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
  ImGuiIO& io = ImGui::GetIO();
  if (io.WantCaptureMouse) {
    return;
  }

  // if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
  //   static double mouse_x, mouse_y;
  //   static int window_width, window_height;

  //   glfwGetCursorPos(window, &mouse_x, &mouse_y);
  //   glfwGetWindowSize(window, &window_width, &window_height);

  //   double norm_x = (mouse_x / window_width * 2.0) - 1.0;
  //   double norm_y = -((mouse_y / window_height * 2.0) - 1.0);
  //   quill::info(logger, "[APPLICATION] Mouse press: pos=({:.0f}, {:.0f}), ndc=({:.2f}, {:.2f})", mouse_x, mouse_y, norm_x, norm_y);
  // }

  if (g_selected_scene_) {
    Mouse mouse = FromGlfwMouse(button);
    if (action == GLFW_PRESS) {
      g_selected_scene_->OnMouseButtonEvent(mouse, true);
    } else if (action == GLFW_RELEASE) {
      g_selected_scene_->OnMouseButtonEvent(mouse, false);
    }
  }
}

void Application::MouseCursorCallback(GLFWwindow* window, double x_pos, double y_pos) {
  ImGuiIO& io = ImGui::GetIO();
  if (io.WantCaptureMouse) {
    return;
  }

  if (g_selected_scene_) {
    g_selected_scene_->OnMouseMoveEvent(x_pos, y_pos);
  }
}

void Application::KeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  ImGuiIO& io = ImGui::GetIO();
  if (io.WantCaptureKeyboard) {
    return;
  }

  if (g_selected_scene_) {
    Key our_key = FromGlfwKey(key);
    if (action == GLFW_PRESS) {
      g_selected_scene_->OnKeyboardEvent(our_key, true);
    } else if (action == GLFW_RELEASE) {
      g_selected_scene_->OnKeyboardEvent(our_key, false);
    }
  }
}

void Application::ScrollCallback(GLFWwindow* window, double x_offset, double y_offset) {
  ImGuiIO& io = ImGui::GetIO();
  if (io.WantCaptureMouse) {
    return;
  }

  if (g_selected_scene_) {
    g_selected_scene_->OnScrollEvent(x_offset, y_offset);
  }
}
