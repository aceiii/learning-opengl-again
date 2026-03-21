#include <array>
#include <print>
#include <string>
#include <string_view>

#include "application.hpp"


namespace {
  const std::array kVertices{
    0.5f, 0.5f, 0.0f,
    0.5f, -0.5f, 0.0f,
    -0.5f, -0.5f, 0.0f,
    -0.5f, 0.5f, 0.0f,
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

  const std::string kFragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    void main() {
      FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
    }
  )";

  unsigned int g_shader_program = 0;
  unsigned int g_vao = 0;
  unsigned int g_vbo = 0;
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
    std::println(stderr, "ERROR: Shader compilation failed [{}]\n{}", type, buffer.data());
  }
}

static void CheckProgramLinkStatus(unsigned int program_id) {
  static std::array<char, 512> buffer;
  buffer.fill(0);

  int success;
  glGetProgramiv(program_id, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(program_id, buffer.size() - 1, nullptr, buffer.data());
    std::println(stderr, "ERROR: Program link failed\n{}", buffer.data());
  }
}

std::expected<void, std::string> Application::Init() {
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

  glViewport(0, 0, 800, 600);

  glfwSetFramebufferSizeCallback(g_window, FrameBufferSizeCallback);
  glfwSetMouseButtonCallback(g_window, MouseButtonCallback);

  unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  auto* vs_source = kVertexShaderSource.c_str();
  glShaderSource(vertex_shader, 1, &vs_source, nullptr);
  glCompileShader(vertex_shader);
  CheckShaderCompilation("VERTEX", vertex_shader);

  unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  auto* fs_source = kFragmentShaderSource.c_str();
  glShaderSource(fragment_shader, 1, &fs_source, nullptr);
  glCompileShader(fragment_shader);
  CheckShaderCompilation("FRAGMENT", fragment_shader);

  g_shader_program = glCreateProgram();
  glAttachShader(g_shader_program, vertex_shader);
  glAttachShader(g_shader_program, fragment_shader);
  glLinkProgram(g_shader_program);
  CheckProgramLinkStatus(g_shader_program);

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
  glEnableVertexAttribArray(0);

  glGenVertexArrays(1, &g_vao);
  glBindVertexArray(g_vao);


  glGenBuffers(1, &g_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  glGenBuffers(1, &g_ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kIndices), kIndices.data(), GL_STATIC_DRAW);

  return {};
}

void Application::Run() {
  while (!glfwWindowShouldClose(g_window)) {
    ProcessInput(g_window);
    glfwSwapBuffers(g_window);
    glfwPollEvents();
  }
}

void Application::Cleanup() {
  glDeleteVertexArrays(1, &g_vao);
  glDeleteBuffers(1, &g_vbo);
  glDeleteProgram(g_shader_program);
  glfwTerminate();
}

void Application::Update() {
}

void Application::Render() {
  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glUseProgram(g_shader_program);
  glBindVertexArray(g_vao);
  // glDrawArrays(GL_TRIANGLES, 0, 3);
  // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void Application::FrameBufferSizeCallback(GLFWwindow* window, int width, int height) {
  glViewport(0, 0, width, height);
}

void Application::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    static double mouse_x, mouse_y;
    static int window_width, window_height;

    glfwGetCursorPos(window, &mouse_x, &mouse_y);
    glfwGetWindowSize(window, &window_width, &window_height);

    double norm_x = (mouse_x / window_width * 2.0) - 1.0;
    double norm_y = -((mouse_y / window_height * 2.0) - 1.0);
    std::println("Mouse click: screen(x={:.0f}, y={:.0f}), normalize=(x={:.2f}, y={:.2f})", mouse_x, mouse_y, norm_x, norm_y);
  }
}

void Application::ProcessInput(GLFWwindow* window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }
}
