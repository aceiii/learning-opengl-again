#pragma once

#include <string>
#include <tuple>
#include <vector>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../image.hpp"
#include "../scene.hpp"
#include "../shader.hpp"


class TransformationsScene final : public Scene {
public:
  void Init(IAppContext* ctx) override {
    textures_.push_back(LoadTexture(GL_TEXTURE0, "resources/textures/container.jpg"));
    textures_.push_back(LoadTexture(GL_TEXTURE1, "resources/textures/awesomeface.png"));

    shader_ = Shader::FromFiles("resources/shaders/transformations_scene/main.vs", "resources/shaders/transformations_scene/main.fs");

    vaos_.resize(1);
    vbos_.resize(1);

    glGenVertexArrays(vaos_.size(), vaos_.data());
    glGenBuffers(vbos_.size(), vbos_.data());

    glGenBuffers(1, &ebo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kIndices), kIndices.data(), GL_STATIC_DRAW);

    glBindVertexArray(vaos_[0]);
    glBindBuffer(GL_ARRAY_BUFFER, vbos_[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
  }

  void Update(float dt) override {
    transform_ = glm::mat4(1.0f);
    if (reverse_transform_) {
      transform_ = glm::rotate(transform_, GetTime(), glm::vec3(0.0f, 0.0f, 1.0f));
      transform_ = glm::translate(transform_, glm::vec3(horizontal_offset_, vertical_offset_, 0.0f));
    } else {
      transform_ = glm::translate(transform_, glm::vec3(horizontal_offset_, vertical_offset_, 0.0f));
      transform_ = glm::rotate(transform_, GetTime(), glm::vec3(0.0f, 0.0f, 1.0f));
    }

    transform2_ = glm::mat4(1.0f);
    transform2_ = glm::translate(transform2_, glm::vec3(-0.5f, 0.5f, 0.f));
    transform2_ = glm::scale(transform2_, glm::vec3(sinf(GetTime())));
  }

  void Render() override {
    glPolygonMode(GL_FRONT_AND_BACK, wireframe_ ? GL_LINE : GL_FILL);

    shader_.Use();
    shader_.SetFloat("textureBlend", texture_blend_);
    shader_.SetInt("texture1", 0);
    shader_.SetInt("texture2", 1);
    shader_.SetMat4("transform", transform_);

    glBindVertexArray(vaos_[0]);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    if (second_container_) {
      shader_.SetMat4("transform", transform2_);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
  }

  void RenderInterface(int window_width, int window_height) override {
    constexpr auto padding = 5.0f;
    constexpr auto menu_bar_height = 32.0f;

    ImGui::PushID("ShadersScene");
    ImGui::SetNextWindowPos(ImVec2(window_width - padding, menu_bar_height - padding), ImGuiCond_FirstUseEver, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Scene Options")) {
      ImGui::Checkbox("Wireframe", &wireframe_);
      ImGui::Checkbox("Show 2nd container", &second_container_);
      ImGui::DragFloat("Texture Blend", &texture_blend_, 0.01f, 0.0f, 1.0f);
      ImGui::DragFloat("X Offset", &horizontal_offset_, 0.01f, -2.0f, 2.0f);
      ImGui::DragFloat("Y Offset", &vertical_offset_, 0.01f, -2.0f, 2.0f);
      ImGui::Checkbox("Reverse transformation order", &reverse_transform_);
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

    shader_.Destroy();

    if (!textures_.empty()) {
      glDeleteTextures(textures_.size(), textures_.data());
    }
    textures_.clear();
  }

  std::string Name() const override {
    return "Transformations";
  }

private:
  unsigned int LoadTexture(GLenum texture, std::string_view path) {
    unsigned texture_id;
    glGenTextures(1, &texture_id);

    glActiveTexture(texture);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    Image image = Image::Load(path);
    if (image.data) {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data.get());
      glGenerateMipmap(GL_TEXTURE_2D);
    }

    return texture_id;
  }

  inline static const std::array kVertices{
     0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
     0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
    -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
    -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f,
  };

  inline static const std::array kIndices{
    0, 1, 3,
    1, 2, 3,
  };

  Shader shader_;
  std::vector<unsigned int> vaos_;
  std::vector<unsigned int> vbos_;
  std::vector<unsigned int> textures_;
  unsigned int ebo_ = 0;

  bool wireframe_ = false;
  bool second_container_ = false;
  bool reverse_transform_ = false;

  float texture_blend_ = 0.2f;
  float vertical_offset_ = -0.5f;
  float horizontal_offset_ = 0.5f;

  glm::mat4 transform_;
  glm::mat4 transform2_;
};
