#pragma once

#include <string>
#include <vector>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include "../scene.hpp"
#include "../shader_util.hpp"


class HelloTriangle final : public Scene {
  public:
  Result Init() override {

  shader_programs_.push_back(util::CreateShaderProgram(kVertexShaderSource, kFragmentShaderSource));
  shader_programs_.push_back(util::CreateShaderProgram(kVertexShaderSource2, kFragmentShaderSource2));

  vaos_.resize(3);
  vbos_.resize(3);

  glGenVertexArrays(vaos_.size(), vaos_.data());
  glGenBuffers(vbos_.size(), vbos_.data());

  glBindVertexArray(vaos_[0]);
  glBindBuffer(GL_ARRAY_BUFFER, vbos_[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 3, kVertices.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  glBindVertexArray(vaos_[1]);
  glBindBuffer(GL_ARRAY_BUFFER, vbos_[1]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 3, kVertices.data() + 9, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  glBindVertexArray(vaos_[2]);
  glBindBuffer(GL_ARRAY_BUFFER, vbos_[2]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(kRectVertices), kRectVertices.data(), GL_STATIC_DRAW);
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

    if (draw_elements_) {
      glUseProgram(shader_programs_[0]);
      glBindVertexArray(vaos_[2]);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    } else {
      glUseProgram(shader_programs_[0]);
      glBindVertexArray(vaos_[0]);
      glDrawArrays(GL_TRIANGLES, 0, 3);

      if (multiple_shaders_) {
        glUseProgram(shader_programs_[1]);
      }
      glBindVertexArray(vaos_[1]);
      glDrawArrays(GL_TRIANGLES, 0, 3);
    }
  }

  void RenderInterface(int window_width, int window_height) override {
    constexpr auto padding = 5.0f;
    constexpr auto menu_bar_height = 32.0f;

    ImGui::PushID("HelloTriangle");
    ImGui::SetNextWindowPos(ImVec2(window_width - padding, menu_bar_height - padding), ImGuiCond_FirstUseEver, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Select")) {
      ImGui::Checkbox("Wireframe", &wireframe_);
      ImGui::Checkbox("Multple Shaders", &multiple_shaders_);
      ImGui::Checkbox("Draw Elements", &draw_elements_);
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
    return "Hello Triangle";
  }

private:
  inline static const std::array kVertices{
    -0.51f, 0.75f, 0.0f,
    -0.21f, -0.02f, 0.0f,
    -0.77f, -0.01f, 0.0f,
    0.47f, 0.76f, 0.0f,
    0.81f, -0.31f, 0.0f,
    0.23f, -0.13f, 0.0f,
  };

  inline static const std::array kRectVertices{
    0.5f,  0.5f, 0.0f,
    0.5f, -0.5f, 0.0f,
    -0.5f, -0.5f, 0.0f,
    -0.5f,  0.5f, 0.0f,
  };

  inline static const std::array kIndices{
    0, 1, 3,
    1, 2, 3,
  };

  inline static const std::string kVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    void main() {
      gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    }
  )";

  inline static const std::string kVertexShaderSource2 = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    void main() {
      gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    }
  )";

  inline static const std::string kFragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    void main() {
      FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
    }
  )";

  inline static const std::string kFragmentShaderSource2 = R"(
    #version 330 core
    out vec4 FragColor;

    void main() {
      FragColor = vec4(0.3f, 0.9f, 0.5f, 1.0f);
    }
  )";

  std::vector<unsigned int> shader_programs_;
  std::vector<unsigned int> vaos_;
  std::vector<unsigned int> vbos_;
  unsigned int ebo_ = 0;

  bool wireframe_ = false;
  bool multiple_shaders_ = true;
  bool draw_elements_ = false;
};
