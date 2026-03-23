#pragma once

#include <fstream>
#include <string>
#include <string_view>
#include <glad/glad.h>

#include "file.hpp"


class Shader {
public:
  unsigned int ID() {
    return id_;
  }

  static Shader FromFiles(std::string_view vs_path, std::string_view fs_path) {
    std::string vs_source = File::ReadContents(std::string{vs_path});
    std::string fs_source = File::ReadContents(std::string{fs_path});
    return Shader(vs_source, fs_source);
  }

  static Shader FromSource(const std::string& vs_source, const std::string& fs_source) {
    return Shader(vs_source, fs_source);
  }

  void Destroy() {
    if (id_) {
      glDeleteShader(id_);
    }
  }

  void Use() {
    glUseProgram(id_);
  }

  void SetBool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(id_, name.data()), static_cast<int>(value));
  }

  void SetInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(id_, name.data()), value);
  }

  void SetFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(id_, name.data()), value);
  }

  void SetFloat4(const std::string& name, std::array<float, 4> arr) const {
    glUniform4f(glGetUniformLocation(id_, name.data()), arr[0], arr[1], arr[2], arr[3]);
  }

  void SetFloat4(const std::string& name, float x, float y, float z, float w) const {
    glUniform4f(glGetUniformLocation(id_, name.data()), x, y, z, w);
  }

public:
  static void CheckShaderCompilation(std::string_view type, unsigned int shader_id) {
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

  static void CheckProgramLinkStatus(unsigned int program_id) {
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

protected:
  Shader(const std::string& vs_source, const std::string& fs_source) {
    id_ = CreateShaderProgram(vs_source, fs_source);
  }

  unsigned int id_ = 0;
};
