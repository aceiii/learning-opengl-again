#pragma once

#include <string>
#include <vector>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include "../scene.hpp"
#include "../shader_util.hpp"


class ShadersScene final : public Scene {
public:
  Result Init() override {
    int num_attributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &num_attributes);
    LogInfo("[SCENE] Maximum num of vertex attributes: {}", num_attributes);

    shader_programs_.push_back(util::CreateShaderProgram(kVertexShaderSource, kFragmentShaderSource));
    shader_programs_.push_back(util::CreateShaderProgram(kVertexShaderSource, kFragmentShaderSource2));

    vaos_.resize(1);
    vbos_.resize(1);

    glGenVertexArrays(vaos_.size(), vaos_.data());
    glGenBuffers(vbos_.size(), vbos_.data());

    glBindVertexArray(vaos_[0]);
    glBindBuffer(GL_ARRAY_BUFFER, vbos_[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &ebo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kIndices), kIndices.data(), GL_STATIC_DRAW);

    return {};
  }

  void Update(float dt) override {
  }

  void Render() override {
    if (wireframe_) {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    if (use_uniform_shader_) {
      float time_value = glfwGetTime();
      float green_value = sin(time_value) / 2.0f + 0.5f;

      glUseProgram(shader_programs_[1]);
      int vertex_color_location = glGetUniformLocation(shader_programs_[1], "ourColor");
      glUniform4f(vertex_color_location, 0.0f, green_value, 0.0f, 1.0f);
    } else {
      glUseProgram(shader_programs_[0]);
    }
    glBindVertexArray(vaos_[0]);
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
  }

  void RenderInterface(int window_width, int window_height) override {
    constexpr auto padding = 5.0f;
    constexpr auto menu_bar_height = 32.0f;

    ImGui::PushID("ShadersScene");
    ImGui::SetNextWindowPos(ImVec2(window_width - padding, menu_bar_height - padding), ImGuiCond_FirstUseEver, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Scene Options")) {
      ImGui::Checkbox("Wireframe", &wireframe_);
      ImGui::Checkbox("Unform Shader", &use_uniform_shader_);
      // ImGui::Checkbox("Draw Elements", &draw_elements_);
    }
    ImGui::End();
    ImGui::PopID();
  }

  void Cleanup() override {
    if (!vaos_.empty()) {
      glDeleteVertexArrays(vaos_.size(), vaos_.data());
      vaos_.clear();
    }

    if (!vbos_.empty()) {
      glDeleteBuffers(vbos_.size(), vbos_.data());
      vbos_.clear();
    }

    if (!shader_programs_.empty()) {
      for (const auto& program_id : shader_programs_) {
        glDeleteProgram(program_id);
      }
      shader_programs_.clear();
    }
  }

  virtual std::string Name() const override {
    return "Shaders";
  }

private:
  inline static const std::array kVertices{
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
     0.0f,  0.5f, 0.0f
  };

  inline static const std::array kIndices{
    0, 1, 2,
  };

  inline static const std::string kVertexShaderSource = R"(
    #version 330 core

    layout (location = 0) in vec3 aPos;

    out vec4 vertexColor;

    void main() {
      gl_Position = vec4(aPos, 1.0);
      vertexColor = vec4(0.5, 0.0, 0.0, 1.0);
    }
  )";

  inline static const std::string kFragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    in vec4 vertexColor;

    void main() {
      FragColor = vertexColor;
    }
  )";

  inline static const std::string kFragmentShaderSource2 = R"(
    #version 330 core
    out vec4 FragColor;

    uniform vec4 ourColor;

    void main() {
      FragColor = ourColor;
    }
  )";

  std::vector<unsigned int> shader_programs_;
  std::vector<unsigned int> vaos_;
  std::vector<unsigned int> vbos_;
  unsigned int ebo_ = 0;

  bool wireframe_ = false;
  bool use_uniform_shader_ = false;
};
