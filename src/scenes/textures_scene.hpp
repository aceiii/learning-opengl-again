#pragma once

#include <string>
#include <vector>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include "../scene.hpp"
#include "../shader.hpp"


class TexturesScene final : public Scene {
public:
  void Init() override {
    shader_ = Shader::FromFiles("resources/shaders/textures_scene/main.vs", "resources/shaders/textures_scene/main.fs");

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glGenBuffers(1, &ebo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kIndices), kIndices.data(), GL_STATIC_DRAW);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
  }

  void Update(float dt) override {
  }

  void Render() override {
    glPolygonMode(GL_FRONT_AND_BACK, wireframe_ ? GL_LINE : GL_FILL);

    shader_.Use();
    shader_.SetFloat("xOffset", horizontal_offset_);
    shader_.SetFloat("yOffset", vertical_offset_);
    glBindVertexArray(vao_);
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
      ImGui::DragFloat("X Offset", &horizontal_offset_, 0.01f, -2.0f, 2.0f);
      ImGui::DragFloat("Y Offset", &vertical_offset_, 0.01f, -2.0f, 2.0f);
    }
    ImGui::End();
    ImGui::PopID();
  }

  void Cleanup() override {
    if (vao_) {
      glDeleteVertexArrays(1, &vao_);
      vao_ = 0;
    }

    if (vbo_) {
      glDeleteBuffers(1, &vbo_);
      vbo_ = 0;
    }

    shader_.Destroy();
  }

  virtual std::string Name() const override {
    return "Textures";
  }

private:
  inline static const std::array kVertices{
     0.5f, -0.5f, 0.0f,   1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,
     0.0f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f,
  };

  inline static const std::array kTexCoords{
    0.0f, 0.0f,
    1.0f, 0.0f,
    0.5f, 1.0f,
  };

  inline static const std::array kIndices{
    0, 1, 2,
  };

  Shader shader_;
  unsigned int vao_ = 0;
  unsigned int vbo_ = 0;
  unsigned int ebo_ = 0;

  bool wireframe_ = false;
  float vertical_offset_ = 0.0f;
  float horizontal_offset_ = 0.0f;

};
