#pragma once

#include <string>
#include <vector>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include "../scene.hpp"
#include "../shader.hpp"


class ShadersScene final : public Scene {
public:
  void Init() override {
    int num_attributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &num_attributes);
    LogInfo("[SCENE] Maximum num of vertex attributes: {}", num_attributes);

    shaders_.push_back(Shader::FromSource(kVertexShaderSource, kFragmentShaderSource));
    shaders_.push_back(Shader::FromSource(kVertexShaderSource, kFragmentShaderSource2));
    shaders_.push_back(Shader::FromSource(kVertexShaderSource2, kFragmentShaderSource3));
    shaders_.push_back(Shader::FromFiles("resources/shaders/shaders_scene/main.vs", "resources/shaders/shaders_scene/main.fs"));

    vaos_.resize(2);
    vbos_.resize(2);

    glGenVertexArrays(vaos_.size(), vaos_.data());
    glGenBuffers(vbos_.size(), vbos_.data());

    glGenBuffers(1, &ebo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kIndices), kIndices.data(), GL_STATIC_DRAW);

    glBindVertexArray(vaos_[0]);
    glBindBuffer(GL_ARRAY_BUFFER, vbos_[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);

    glBindVertexArray(vaos_[1]);
    glBindBuffer(GL_ARRAY_BUFFER, vbos_[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kVerticesWithColors), kVerticesWithColors.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
  }

  void Update(float dt) override {
  }

  void Render() override {
    if (wireframe_) {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    float time_value = glfwGetTime();
    float green_value = sin(time_value) / 2.0f + 0.5f;

    switch (mode_) {
    case 0:
      shaders_[0].Use();
      glBindVertexArray(vaos_[0]);
      break;
    case 1:
      shaders_[1].Use();
      shaders_[1].SetFloat4("ourColor", {0.0f, green_value, 0.0f, 1.0f});
      glBindVertexArray(vaos_[0]);
      break;
    case 2:
      shaders_[2].Use();
      glBindVertexArray(vaos_[1]);
      break;
    case 3:
      shaders_[3].Use();
      shaders_[3].SetFloat("yScale", flip_y_ ? -1.0f : 1.0f);
      shaders_[3].SetFloat("xOffset", horizontal_offset_);
      shaders_[3].SetFloat("colorMix", vert_pos_color_ ? 1.0f : 0.0f);
      glBindVertexArray(vaos_[1]);
      break;
    }

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

      if (ImGui::BeginCombo("Mode", kModeNames[mode_])) {
        for (int i = 0; i < kModeNames.size(); i++) {
          if (ImGui::Selectable(kModeNames[i], i == mode_)) {
            mode_ = i;
          }
        }
        ImGui::EndCombo();
      }

      ImGui::NewLine();

      if (mode_ == 3) {
        ImGui::Checkbox("Flip Y", &flip_y_);
        ImGui::DragFloat("X Offset", &horizontal_offset_, 0.01f, -2.0f, 2.0f);
        ImGui::Checkbox("Vertex Position Color", &vert_pos_color_);
      }
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

    for (auto& shader : shaders_) {
      shader.Destroy();
    }
    shaders_.clear();
  }

  virtual std::string Name() const override {
    return "Shaders";
  }

private:
  inline static const std::array kVertices{
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
     0.0f,  0.5f, 0.0f,
  };

  inline static const std::array kVerticesWithColors{
     0.5f, -0.5f, 0.0f,   1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,
     0.0f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f,
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

  inline static const std::string kVertexShaderSource2 = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aColor;

    out vec3 ourColor;

    void main() {
      gl_Position = vec4(aPos, 1.0);
      ourColor = aColor;
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


  inline static const std::string kFragmentShaderSource3 = R"(
    #version 330 core
    out vec4 FragColor;
    in vec3 ourColor;

    void main() {
      FragColor = vec4(ourColor, 1.0);
    }
  )";

  std::vector<Shader> shaders_;
  std::vector<unsigned int> vaos_;
  std::vector<unsigned int> vbos_;
  unsigned int ebo_ = 0;

  bool wireframe_ = false;

  inline static const std::array kModeNames{
    "Vertex shader color",
    "Unform shader",
    "Vertex colors",
    "Shaders from file",
  };

  int mode_ = 0;

  bool flip_y_ = false;
  float horizontal_offset_ = 0.0f;
  bool vert_pos_color_ = false;

};
