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

  glfwSetFramebufferSizeCallback(g_window, FrameBufferSizeCallback);
  glfwSetMouseButtonCallback(g_window, MouseButtonCallback);

  ImGuiImpl::Init(g_window);

  glViewport(0, 0, 800, 600);

  scenes_.push_back(EmptyScene::Get());
  scenes_.push_back(std::make_shared<HelloTriangle>());
  scenes_.push_back(std::make_shared<ShadersScene>());
  scenes_.push_back(std::make_shared<TexturesScene>());
  scenes_.push_back(std::make_shared<TransformationsScene>());
  scenes_.push_back(std::make_shared<CoordinateSystemsScene>());
  scenes_.push_back(std::make_shared<CameraScene>());

  return {};
}

void Application::Run() {
  double last_frame_time = glfwGetTime();

  while (!glfwWindowShouldClose(g_window)) {
    double current_time = glfwGetTime();
    double delta_time = current_time - last_frame_time;
    last_frame_time = current_time;

    ProcessInput(g_window);
    Update(delta_time);
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

void Application::Update(float dt) {
  if (selected_scene_) {
    selected_scene_->Update(dt);
  }
}

void Application::Render() {
  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (selected_scene_) {
    selected_scene_->Render();
  }
}

void Application::RenderInterface() {
  static bool show_logs = true;
  static bool show_demo_window = false;
  static int window_width, window_height;

  glfwGetWindowSize(g_window, &window_width, &window_height);

  ImGuiImpl::NewFrame();
  {
    ImGui::BeginMainMenuBar();
    {
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
      ImGui::SetNextWindowPos(ImVec2(padding, window_height - padding), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
      ImGui::SetNextWindowSize(ImVec2(window_width - padding - padding, 180.0f - padding - padding), ImGuiCond_Always);
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
        if (ImGui::BeginCombo("Scene", selected_scene_->Name().c_str())) {
          for (const auto& scene : scenes_) {
            const bool is_selected = scene == selected_scene_;
            if (ImGui::Selectable(scene->Name().c_str(), is_selected)) {
              quill::info(logger, "[APPLICATION] Selected scene: {}", scene->Name());

              if (!is_selected) {
                selected_scene_->Cleanup();
                selected_scene_ = scene;
                selected_scene_->Init();
              }
            }
          }
          ImGui::EndCombo();
        }
      }
      ImGui::End();
    }

    if (selected_scene_) {
      selected_scene_->RenderInterface(window_width, window_height);
    }
  }
  ImGui::Render();
}

void Application::LogCallback(std::string_view message) {
  app_log_.AddLog(message);
}

void Application::ErrorCallback(int error_code, const char* description) {
  quill::error(logger, "[GLFW] code={}, description={}", error_code, description);
}

void Application::FrameBufferSizeCallback(GLFWwindow* window, int width, int height) {
  glViewport(0, 0, width, height);
}

void Application::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
  ImGuiIO& io = ImGui::GetIO();
  if (io.WantCaptureMouse) {
    return;
  }

  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    static double mouse_x, mouse_y;
    static int window_width, window_height;

    glfwGetCursorPos(window, &mouse_x, &mouse_y);
    glfwGetWindowSize(window, &window_width, &window_height);

    double norm_x = (mouse_x / window_width * 2.0) - 1.0;
    double norm_y = -((mouse_y / window_height * 2.0) - 1.0);
    quill::info(logger, "[APPLICATION] Mouse press: pos=({:.0f}, {:.0f}), ndc=({:.2f}, {:.2f})", mouse_x, mouse_y, norm_x, norm_y);
  }
}

void Application::ProcessInput(GLFWwindow* window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }
}
