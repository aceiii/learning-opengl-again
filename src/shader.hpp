#pragma once

#include <string>
#include <string_view>
#include <glad/glad.h>


class Shader {
public:
  unsigned int id = 0;

  Shader(std::string_view vs_path, std::string_view fs_path) {
    std::string vs_source = "";
    std::string fs_source = "";

    id = CreateShaderProgram(vs_source, fs_source);
  }

  void Use() {
    glUseProgram(id);
  }

  void SetBool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(id, name.data()), static_cast<int>(value));
  }

  void SetInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(id, name.data()), value);
  }

  void SetFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(id, name.data()), value);
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

};
