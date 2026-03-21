#include <array>
#include <print>
#include <string>
#include <string_view>
#include <glad/glad.h>
#include <GLFW/glfw3.h>


namespace {
  const std::array vertices{
    -0.5f, -0.5f, 0.0f,
    0.5f, -0.5f, 0.0f,
    0.0f,  0.5f, 0.0f
  };

  const char* vertex_shader_source = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    void main() {
      gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    }
  )";

  const char* fragment_shader_source = R"(
    #version 330 core
    out vec4 FragColor;

    void main() {
      FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
    }
  )";
}

void FrameBufferSizeCallback(GLFWwindow* window, int width, int height) {
  glViewport(0, 0, width, height);
}

void ProcessInput(GLFWwindow* window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }
}

void CheckShaderCompilation(std::string_view type, unsigned int shader_id) {
  static std::array<char, 512> buffer;
  buffer.fill(0);

  int success;
  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shader_id, buffer.size() - 1, nullptr, buffer.data());
    std::println(stderr, "ERROR: Shader compilation failed [{}]\n{}", type, buffer.data());
  }
}

void CheckProgramLinkStatus(unsigned int program_id) {
  static std::array<char, 512> buffer;
  buffer.fill(0);

  int success;
  glGetProgramiv(program_id, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(program_id, buffer.size() - 1, nullptr, buffer.data());
    std::println(stderr, "ERROR: Program link failed\n{}", buffer.data());
  }
}

auto main() -> int {
  std::println("Hello, world!!");

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  GLFWwindow* window = glfwCreateWindow(800, 600, "Learning OpenGL AGAIN!", nullptr, nullptr);
  if (window == nullptr) {
    std::println(stderr, "Failed to create GLFW Window");
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::println(stderr, "Failed to initialize GLAD");
    return -1;
  }

  glViewport(0, 0, 800, 600);

  glfwSetFramebufferSizeCallback(window, FrameBufferSizeCallback);

  unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_source, nullptr);
  glCompileShader(vertex_shader);
  CheckShaderCompilation("VERTEX", vertex_shader);

  unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_source, nullptr);
  glCompileShader(fragment_shader);
  CheckShaderCompilation("FRAGMENT", fragment_shader);

  unsigned int shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgram(shader_program);
  CheckProgramLinkStatus(shader_program);

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
  glEnableVertexAttribArray(0);

  unsigned int vbo;
  glGenBuffers(1, &vbo);

  unsigned int vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  while (!glfwWindowShouldClose(window)) {
    ProcessInput(window);

    {
      glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      glUseProgram(shader_program);
      glBindVertexArray(vao);
      glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();

  return 0;
}

