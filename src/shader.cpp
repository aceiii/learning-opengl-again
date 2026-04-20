#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "file.hpp"
#include "shader.hpp"
#include "logger.hpp"


Shader Shader::FromFiles(std::string_view gs_path, std::string_view vs_path, std::string_view fs_path) {
  std::string gs_source = File::ReadContents(std::string{gs_path});
  std::string vs_source = File::ReadContents(std::string{vs_path});
  std::string fs_source = File::ReadContents(std::string{fs_path});
  return Shader(vs_source, fs_source);
}

Shader Shader::FromFiles(std::string_view vs_path, std::string_view fs_path) {
  std::string vs_source = File::ReadContents(std::string{vs_path});
  std::string fs_source = File::ReadContents(std::string{fs_path});
  return Shader(vs_source, fs_source);
}

Shader Shader::FromSource(const std::string& vs_source, const std::string& fs_source) {
  return Shader(vs_source, fs_source);
}

void Shader::CheckShaderCompilation(std::string_view type, unsigned int shader_id) {
  static std::array<char, 512> buffer;
  buffer.fill(0);

  int success;
  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shader_id, buffer.size() - 1, nullptr, buffer.data());
    auto* logger = Logger::GetRootLogger();
    quill::warning(logger, "[SHADER] Shader compilation failed [{}]\n{}", type, buffer.data());
  }
}

void Shader::CheckProgramLinkStatus(unsigned int program_id) {
  static std::array<char, 512> buffer;
  buffer.fill(0);

  int success;
  glGetProgramiv(program_id, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(program_id, buffer.size() - 1, nullptr, buffer.data());
    auto* logger = Logger::GetRootLogger();
    quill::warning(logger, "[SHADER]: Program link failed\n{}", buffer.data());
  }
}

unsigned int Shader::CreateShaderProgram(const std::string& vertex_shader_source, const std::string& fragment_shader_source) {
  return CreateShaderProgram("", vertex_shader_source, fragment_shader_source);
}

unsigned int Shader::CreateShaderProgram(const std::string& geometry_shader_source, const std::string& vertex_shader_source, const std::string& fragment_shader_source) {
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


Shader::Shader() : id_{0} {}

Shader::Shader(const std::string& vs_source, const std::string& fs_source) {
  id_ = CreateShaderProgram(vs_source, fs_source);
}

Shader::Shader(const std::string& gs_source, const std::string& vs_source, const std::string& fs_source) {
  id_ = CreateShaderProgram(gs_source, vs_source, fs_source);
}

unsigned int Shader::ID() {
  return id_;
}

void Shader::Destroy() {
  if (id_) {
    glDeleteShader(id_);
    id_ = 0;
  }
}

void Shader::Use() {
  glUseProgram(id_);
}

void Shader::SetBlockBinding(const std::string& name, unsigned int binding) {
  glUniformBlockBinding(id_, glGetUniformBlockIndex(id_, name.data()), binding);
}

void Shader::SetBool(const std::string& name, bool value) {
  glUniform1i(glGetUniformLocation(id_, name.data()), static_cast<int>(value));
}

void Shader::SetInt(const std::string& name, int value) {
  glUniform1i(glGetUniformLocation(id_, name.data()), value);
}

void Shader::SetFloat(const std::string& name, float value) {
  glUniform1f(glGetUniformLocation(id_, name.data()), value);
}

void Shader::SetFloat4(const std::string& name, std::array<float, 4> arr) {
  glUniform4f(glGetUniformLocation(id_, name.data()), arr[0], arr[1], arr[2], arr[3]);
}

void Shader::SetFloat4(const std::string& name, float x, float y, float z, float w) {
  glUniform4f(glGetUniformLocation(id_, name.data()), x, y, z, w);
}

void Shader::SetMat4(const std::string& name, const glm::mat4& matrix) {
  glUniformMatrix4fv(glGetUniformLocation(id_, name.data()), 1, GL_FALSE, glm::value_ptr(matrix));
}

void Shader::SetVec3(const std::string& name, float x, float y, float z) {
  glUniform3f(glGetUniformLocation(id_, name.data()), x, y, z);
}

void Shader::SetVec3(const std::string& name, const glm::vec3& vec) {
  glUniform3fv(glGetUniformLocation(id_, name.data()), 1, glm::value_ptr(vec));
}

void Shader::SetIntSpan(const std::string& name, std::span<const int> span) {
  glUniform1iv(glGetUniformLocation(id_, name.data()), span.size(), span.data());
}

void Shader::SetFloatSpan(const std::string& name, std::span<const float> span) {
  glUniform1fv(glGetUniformLocation(id_, name.data()), span.size(), span.data());
}
