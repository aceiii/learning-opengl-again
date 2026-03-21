#include <array>
#include <print>
#include <string>
#include <string_view>
#include <quill/SimpleSetup.h>
#include <quill/LogFunctions.h>
#include "application.hpp"
#include "imgui_impl.hpp"

namespace {
  quill::Logger* logger = quill::simple_logger();

  const std::array kVertices{
    -0.51f, 0.75f, 0.0f,
    -0.21f, -0.02f, 0.0f,
    -0.77f, -0.01f, 0.0f,
    0.47f, 0.76f, 0.0f,
    0.81f, -0.31f, 0.0f,
    0.23f, -0.13f, 0.0f,
  };

  const std::array kIndices{
    0, 1, 3,
    1, 2, 3,
  };

  const std::string kVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    void main() {
      gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    }
  )";

  const std::string kVertexShaderSource2 = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    void main() {
      gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    }
  )";

  const std::string kFragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    void main() {
      FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
    }
  )";

  const std::string kFragmentShaderSource2 = R"(
    #version 330 core
    out vec4 FragColor;

    void main() {
      FragColor = vec4(0.3f, 0.9f, 0.5f, 1.0f);
    }
  )";

  std::array<unsigned int, 2> g_shader_programs;
  std::array<unsigned int, 2> g_vaos;
  std::array<unsigned int, 2> g_vbos;
  unsigned int g_ebo = 0;

  GLFWwindow* g_window = nullptr;
}

static void CheckShaderCompilation(std::string_view type, unsigned int shader_id) {
  static std::array<char, 512> buffer;
  buffer.fill(0);

  int success;
  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shader_id, buffer.size() - 1, nullptr, buffer.data());
    quill::warning(logger, "[SHADER] Shader compilation failed [{}]\n{}", type, buffer.data());
  }
}

static void CheckProgramLinkStatus(unsigned int program_id) {
  static std::array<char, 512> buffer;
  buffer.fill(0);

  int success;
  glGetProgramiv(program_id, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(program_id, buffer.size() - 1, nullptr, buffer.data());
    quill::warning(logger, "ERROR: Program link failed\n{}", buffer.data());
  }
}

static unsigned int CreateShaderProgram(const std::string& vertex_shader_source, const std::string& fragment_shader_source) {
  auto* vs_source = vertex_shader_source.c_str();
  unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vs_source, nullptr);
  glCompileShader(vertex_shader);
  CheckShaderCompilation("vertex", vertex_shader);

  auto* fs_source = fragment_shader_source.c_str();
  unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fs_source, nullptr);
  glCompileShader(fragment_shader);
  CheckShaderCompilation("fragment", fragment_shader);

  unsigned int program_id = glCreateProgram();
  glAttachShader(program_id, vertex_shader);
  glAttachShader(program_id, fragment_shader);
  glLinkProgram(program_id);
  CheckProgramLinkStatus(program_id);

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  return program_id;
}

std::expected<void, std::string> Application::Init() {
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

  g_shader_programs[0] = CreateShaderProgram(kVertexShaderSource, kFragmentShaderSource);
  g_shader_programs[1] = CreateShaderProgram(kVertexShaderSource2, kFragmentShaderSource2);

  glGenVertexArrays(g_vaos.size(), g_vaos.data());
  glGenBuffers(g_vbos.size(), g_vbos.data());

  glBindVertexArray(g_vaos[0]);
  glBindBuffer(GL_ARRAY_BUFFER, g_vbos[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 3, kVertices.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  glBindVertexArray(g_vaos[1]);
  glBindBuffer(GL_ARRAY_BUFFER, g_vbos[1]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 3, kVertices.data() + 9, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  // glGenBuffers(1, &g_ebo);
  // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ebo);
  // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kIndices), kIndices.data(), GL_STATIC_DRAW);

  return {};
}

void Application::Run() {
  while (!glfwWindowShouldClose(g_window)) {
    ProcessInput(g_window);
    Update();
    Render();
    RenderInterface();

    ImGuiImpl::RenderDrawData();
    glfwSwapBuffers(g_window);
    glfwPollEvents();
  }
}

void Application::Cleanup() {
  quill::info(logger, "[APPLICATION] Cleaning up application");

  glDeleteVertexArrays(g_vaos.size(), g_vaos.data());
  g_vaos.fill(0);

  glDeleteBuffers(g_vbos.size(), g_vbos.data());
  g_vbos.fill(0);

  for (const auto& program_id : g_shader_programs) {
    glDeleteProgram(program_id);
  }
  g_shader_programs.fill(0);

  glfwTerminate();
}

void Application::Update() {
}

void Application::Render() {
  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glUseProgram(g_shader_programs[0]);
  glBindVertexArray(g_vaos[0]);
  glDrawArrays(GL_TRIANGLES, 0, 3);

  glUseProgram(g_shader_programs[1]);
  glBindVertexArray(g_vaos[1]);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void Application::RenderInterface() {
  static bool show_logs = true;
  static bool show_demo_window = false;
  static int window_width, window_height;

  glfwGetWindowSize(g_window, &window_width, &window_height);

  ImGuiImpl::NewFrame();

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
    if (ImGui::Begin("Logs", &show_logs, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove)) {
      app_log_.Draw();
      ImGui::End();
    }
  }

  ImGui::Render();
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
