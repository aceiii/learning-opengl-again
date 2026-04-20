#pragma once

#include <span>
#include <string>
#include <string_view>
#include <glm/glm.hpp>

#include "file.hpp"


class Shader {
public:
  static Shader FromFiles(std::string_view gs_path, std::string_view vs_path, std::string_view fs_path);
  static Shader FromFiles(std::string_view vs_path, std::string_view fs_path);
  static Shader FromSource(const std::string& vs_source, const std::string& fs_source);
  static void CheckShaderCompilation(std::string_view type, unsigned int shader_id);
  static void CheckProgramLinkStatus(unsigned int program_id);
  static unsigned int CreateShaderProgram(const std::string& vertex_shader_source, const std::string& fragment_shader_source);
  static unsigned int CreateShaderProgram(const std::string& geometry_shader_source, const std::string& vertex_shader_source, const std::string& fragment_shader_source);

  Shader();

  unsigned int ID();
  void Destroy();
  void Use();

  void SetBlockBinding(const std::string& name, unsigned int binding);
  void SetBool(const std::string& name, bool value);
  void SetInt(const std::string& name, int value);
  void SetFloat(const std::string& name, float value);
  void SetFloat4(const std::string& name, std::array<float, 4> arr);
  void SetFloat4(const std::string& name, float x, float y, float z, float w);
  void SetMat4(const std::string& name, const glm::mat4& matrix);
  void SetVec3(const std::string& name, float x, float y, float z);
  void SetVec3(const std::string& name, const glm::vec3& vec);
  void SetIntSpan(const std::string& name, std::span<const int> span);
  void SetFloatSpan(const std::string& name, std::span<const float> span);

private:
  Shader(const std::string& vs_source, const std::string& fs_source);
  Shader(const std::string& gs_source, const std::string& vs_source, const std::string& fs_source);

  unsigned int id_ = 0;
};
