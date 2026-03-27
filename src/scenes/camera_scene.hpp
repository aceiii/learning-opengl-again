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


class CameraScene final : public Scene {
public:
  void Init() override {
    textures_.push_back(LoadTexture(GL_TEXTURE0, "resources/textures/container.jpg"));
    textures_.push_back(LoadTexture(GL_TEXTURE1, "resources/textures/awesomeface.png"));

    shader_ = Shader::FromFiles("resources/shaders/coordinate_systems_scene/main.vs", "resources/shaders/coordinate_systems_scene/main.fs");

    vaos_.resize(1);
    vbos_.resize(1);

    glGenVertexArrays(vaos_.size(), vaos_.data());
    glGenBuffers(vbos_.size(), vbos_.data());

    // glGenBuffers(1, &ebo_);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kIndices), kIndices.data(), GL_STATIC_DRAW);

    glBindVertexArray(vaos_[0]);
    glBindBuffer(GL_ARRAY_BUFFER, vbos_[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);

    glEnable(GL_DEPTH_TEST);

    camera_pos_ = glm::vec3(0.0f, 0.0f, 3.0f);
    camera_target_ = glm::vec3(0.0f, 0.0f, 0.0f);
  }

  void Update(float dt) override {
    // glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    // glm::vec3 camera_direction = glm::normalize(camera_pos_ - camera_target_);
    // glm::vec3 camera_right = glm::normalize(glm::cross(up, camera_direction));
    // glm::vec3 camera_up = glm::cross(camera_direction, camera_right);

    float cam_x = sin(GetTime()) * camera_radius_;
    float cam_z = cos(GetTime()) * camera_radius_;
    view_ = glm::lookAt(glm::vec3(cam_x, 0.0f, cam_z), camera_target_, glm::vec3(0.0f, 1.0f, 0.0f));
    projection_ = glm::perspective(glm::radians(fov_), aspect_ratio_, 0.1f, 100.0f);
  }

  void Render() override {
    glPolygonMode(GL_FRONT_AND_BACK, wireframe_ ? GL_LINE : GL_FILL);

    shader_.Use();
    shader_.SetInt("texture1", 0);
    shader_.SetInt("texture2", 1);
    shader_.SetMat4("view", view_);
    shader_.SetMat4("projection", projection_);

    glBindVertexArray(vaos_[0]);
    for (unsigned int i = 0; i < 10; i++) {
      const float angle = 20.0f * i;
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, kCubePositions[i]);
      model = glm::rotate(model, glm::radians(angle + GetTime() * kCubeRotationMultpliers[i]), glm::vec3(1.0f, 0.3f, 0.5f));
      shader_.SetMat4("model", model);

      glDrawArrays(GL_TRIANGLES, 0, 36);
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
      // ImGui::DragFloat3("Camera Pos", &camera_pos_.r, 0.1f, -10.0f, 10.f);
      ImGui::DragFloat3("Camera Target", &camera_target_[0], 0.1f, -10.0f, 10.f);
      ImGui::DragFloat("Camera Radius", &camera_radius_, 0.0f, 0.5f, 100.0f);
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

  virtual std::string Name() const override {
    return "Camera System";
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
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
  };

  inline static const std::array kCubePositions{
    glm::vec3( 0.0f,  0.0f,  0.0f),
    glm::vec3( 2.0f,  5.0f, -15.0f),
    glm::vec3(-1.5f, -2.2f, -2.5f),
    glm::vec3(-3.8f, -2.0f, -12.3f),
    glm::vec3( 2.4f, -0.4f, -3.5f),
    glm::vec3(-1.7f,  3.0f, -7.5f),
    glm::vec3( 1.3f, -2.0f, -2.5f),
    glm::vec3( 1.5f,  2.0f, -2.5f),
    glm::vec3( 1.5f,  0.2f, -1.5f),
    glm::vec3(-1.3f,  1.0f, -1.5f),
  };

  inline static const std::array kCubeRotationMultpliers{
     13.5f,
    -35.0f,
     5.77f,
     9.31f,
    -11.1f,
     4.7f,
     28.8f,
    -21.0f,
    -14.9f,
     7.55f,
  };

  Shader shader_;
  std::vector<unsigned int> vaos_;
  std::vector<unsigned int> vbos_;
  std::vector<unsigned int> textures_;
  unsigned int ebo_ = 0;

  bool wireframe_ = false;

  float fov_ = 45.0f;
  float aspect_ratio_ = 800.0f / 600.0f;
  float camera_radius_ = 10.0f;

  glm::mat4 view_;
  glm::mat4 projection_;

  glm::vec3 camera_pos_;
  glm::vec3 camera_target_;
};
