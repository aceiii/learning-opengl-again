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
#include "../camera.hpp"


class MaterialsScene final : public Scene {
public:
  void Init(IAppContext* ctx) override {
    ctx_ = ctx;

    lighting_shader_ = Shader::FromFiles("resources/shaders/materials_scene/main.vs", "resources/shaders/materials_scene/main.fs");
    light_cube_shader_ = Shader::FromFiles("resources/shaders/materials_scene/light.vs", "resources/shaders/materials_scene/light.fs");

    vaos_.resize(2);
    vbos_.resize(1);

    glGenVertexArrays(vaos_.size(), vaos_.data());
    glGenBuffers(vbos_.size(), vbos_.data());

    glBindVertexArray(vaos_[0]);
    glBindBuffer(GL_ARRAY_BUFFER, vbos_[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(vaos_[1]);
    glBindBuffer(GL_ARRAY_BUFFER, vbos_[0]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);
    glEnableVertexAttribArray(0);

    glEnable(GL_DEPTH_TEST);

    camera_.position = glm::vec3(0.82, 0.85f, 4.1f);
    camera_.yaw = -100.0f;
    camera_.pitch = -10.0f;
    camera_.UpdateCameraVectors();
  }

  void Update(float dt) override {
    if (ctx_->IsKeyDown(Key::kKeyUp) || ctx_->IsKeyDown(Key::kKeyW)) {
      camera_.ProcessKeyboard(CameraMovement::kMoveForward, dt);
    }
    if (ctx_->IsKeyDown(Key::kKeyDown) || ctx_->IsKeyDown(Key::kKeyS)) {
      camera_.ProcessKeyboard(CameraMovement::kMoveBackward, dt);
    }
    if (ctx_->IsKeyDown(Key::kKeyLeft) || ctx_->IsKeyDown(Key::kKeyA)) {
      camera_.ProcessKeyboard(CameraMovement::kMoveLeft, dt);
    }
    if (ctx_->IsKeyDown(Key::kKeyRight) || ctx_->IsKeyDown(Key::kKeyD)) {
      camera_.ProcessKeyboard(CameraMovement::kMoveRight, dt);
    }

    projection_ = glm::perspective(glm::radians(camera_.fov), aspect_ratio_, 0.1f, 100.0f);

    if (animate_light_color_) {
      glm::vec3 lightColor;
      lightColor.x = sin(ctx_->GetTime() * 2.0f);
      lightColor.y = sin(ctx_->GetTime() * 0.7f);
      lightColor.z = sin(ctx_->GetTime() * 1.3f);

      light_.diffuse = lightColor * glm::vec3(0.5f);
      light_.ambient = light_.diffuse * 0.2f;
    }
  }

  void Render() override {
    glPolygonMode(GL_FRONT_AND_BACK, wireframe_ ? GL_LINE : GL_FILL);

    if (animate_light_pos_) {
      glm::mat4 light_transform = glm::mat4(1.0f);
      light_transform = glm::rotate(light_transform, ctx_->GetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
      light_transform = glm::translate(light_transform, glm::vec3(1.0f, 1.0f, 3.0f));
      light_.position = light_transform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    glm::mat4 view = camera_.GetViewMatrix();

    lighting_shader_.Use();
    lighting_shader_.SetMat4("view", view);
    lighting_shader_.SetMat4("projection", projection_);
    lighting_shader_.SetMat4("model", glm::mat4(1.0f));
    lighting_shader_.SetVec3("viewPos", camera_.position);
    lighting_shader_.SetVec3("light.position", light_.position);
    lighting_shader_.SetVec3("light.ambient", light_.ambient);
    lighting_shader_.SetVec3("light.diffuse", light_.diffuse);
    lighting_shader_.SetVec3("light.specular", light_.specular);
    lighting_shader_.SetVec3("material.ambient", material_.ambient);
    lighting_shader_.SetVec3("material.diffuse", material_.diffuse);
    lighting_shader_.SetVec3("material.specular", material_.specular);
    lighting_shader_.SetFloat("material.shininess", material_.shininess);

    glBindVertexArray(vaos_[0]);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, light_.position);
    model = glm::scale(model, glm::vec3(0.2f));

    light_cube_shader_.Use();
    light_cube_shader_.SetMat4("view", view);
    light_cube_shader_.SetMat4("projection", projection_);
    light_cube_shader_.SetMat4("model", model);
    light_cube_shader_.SetVec3("lightColor", light_.diffuse);

    glBindVertexArray(vaos_[0]);
    glDrawArrays(GL_TRIANGLES, 0, 36);
  }

  void RenderInterface(int window_width, int window_height) override {
    constexpr auto padding = 5.0f;
    constexpr auto menu_bar_height = 32.0f;

    ImGui::PushID("Materials");
    ImGui::SetNextWindowPos(ImVec2(window_width - padding, menu_bar_height - padding), ImGuiCond_FirstUseEver, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Scene Options")) {
      ImGui::Checkbox("Wireframe", &wireframe_);
      ImGui::NewLine();
      if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Lighting");
        ImGui::Checkbox("Animate light color", &animate_light_color_);
        ImGui::ColorEdit3("Ambient", &light_.ambient[0]);
        ImGui::ColorEdit3("Diffuse", &light_.diffuse[0]);
        ImGui::ColorEdit3("Specular", &light_.specular[0]);
        ImGui::Checkbox("Animate light position", &animate_light_pos_);
        if (!animate_light_pos_) {
          ImGui::DragFloat3("Light position", &light_.position[0], 0.1f);
        }
        ImGui::PopID();
      }
      if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Material");
        ImGui::ColorEdit3("Ambient", &material_.ambient[0]);
        ImGui::ColorEdit3("Diffuse", &material_.diffuse[0]);
        ImGui::ColorEdit3("Specular", &material_.specular[0]);
        ImGui::DragFloat("Shininess", &material_.shininess, 0.01f, 0.01f, 10.0f);
        ImGui::PopID();
      }
      if (ImGui::CollapsingHeader("Camera")) {
        ImGui::Checkbox("Hide UI During Capture", &hide_interface_);
        ImGui::Checkbox("Hold Capture on mouse press", &capture_hold_);
        ImGui::DragFloat("Field of View", &camera_.fov, 0.1f, Camera::kMinFov, Camera::kMaxFov);
        ImGui::DragFloat("Camera Speed", &camera_.movement_speed, 0.01f, Camera::kMinSpeed, Camera::kMaxSpeed);
        ImGui::DragFloat3("Camera Pos", &camera_.position[0], 0.1f, -10.0f, 10.f);
        ImGui::DragFloat3("Camera Front", &camera_.front[0], 0.01f, -1.0f, 1.0f);
        if (ImGui::DragFloat("Yaw", &camera_.yaw, 0.01f, Camera::kMinYaw, Camera::kMaxYaw)) {
          camera_.UpdateCameraVectors();
        }
        if (ImGui::DragFloat("Pitch", &camera_.pitch, 0.01f, Camera::kMinPitch, Camera::kMaxPitch)) {
          camera_.UpdateCameraVectors();
        }
      }
    }
    ImGui::End();
    ImGui::PopID();
  }

  void Cleanup() override {
    ctx_ = nullptr;

    if (!vaos_.empty()) {
      glDeleteVertexArrays(vaos_.size(), vaos_.data());
      vaos_.clear();
    }

    if (!vbos_.empty()) {
      glDeleteBuffers(vbos_.size(), vbos_.data());
      vbos_.clear();
    }

    lighting_shader_.Destroy();
  }

  std::string Name() const override {
    return "Materials";
  }

  void OnMouseMoveEvent(float x, float y) override {
    if (reset_mouse_) {
      last_mouse_.x = x;
      last_mouse_.y = y;
      reset_mouse_ = false;
    }

    float offset_x = x - last_mouse_.x;
    float offset_y = y - last_mouse_.y;

    last_mouse_.x = x;
    last_mouse_.y = y;

    if (capture_mouse_) {
      camera_.ProcessMouseMovement(offset_x, offset_y);
    }
  }

  void OnMouseButtonEvent(Mouse mouse, bool pressed) override {
    if (mouse == Mouse::kMouseLeft && pressed && !capture_mouse_) {
      ToggleCaptureMouse(true);
    } else if (!capture_hold_ && mouse == Mouse::kMouseLeft && !pressed && capture_mouse_) {
      ToggleCaptureMouse(false);
    }
  }

  void OnScrollEvent(float x, float y) override {
    camera_.ProcessMouseScroll(y);
  }

  void OnKeyboardEvent(Key key, bool pressed) override {
    if (key == Key::kKeyEscape && pressed) {
      ToggleCaptureMouse(false);
    }
  }

private:
  void ToggleCaptureMouse(bool capture) {
    if (capture) {
      ctx_->CaptureMouse(true);
      if (hide_interface_) {
        ctx_->ToggleUI(false);
      }
      capture_mouse_ = true;
      reset_mouse_ = true;
    } else {
      ctx_->CaptureMouse(false);
      ctx_->ToggleUI(true);
      capture_mouse_ = false;
      reset_mouse_ = true;
    }
  }

  struct Material {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
  };

  struct Light {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
  };

  inline static const std::array kVertices{

    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
  };

  IAppContext* ctx_ = nullptr;

  inline static const float kDefaultYaw = -90.0f;
  inline static const float kDefaultPitch = 0.0f;
  inline static const float kMinPitch = -89.0f;
  inline static const float kMaxPitch = 89.0f;


  Shader lighting_shader_;
  Shader light_cube_shader_;

  std::vector<unsigned int> vaos_;
  std::vector<unsigned int> vbos_;

  bool wireframe_ = false;
  bool auto_rotate_camera_ = false;
  bool capture_mouse_ = false;
  bool capture_hold_ = false;
  bool reset_mouse_ = true;
  bool hide_interface_ = true;
  bool animate_light_pos_ = false;
  bool animate_light_color_ = false;

  float aspect_ratio_ = 800.0f / 600.0f;
  float camera_radius_ = 10.0f;

  Camera camera_{glm::vec3(0.0f, 0.0f, 3.0f)};
  Material material_{
    .ambient = glm::vec3(1.0f, 0.5f, 0.31f),
    .diffuse = glm::vec3(1.0f, 0.5f, 0.31f),
    .specular = glm::vec3(0.5f, 0.5f, 0.5f),
    .shininess = 32.0f,
  };
  Light light_{
    .position = glm::vec3(1.0f, 1.0f, 2.0f),
    .ambient = glm::vec3(0.2f, 0.2f, 0.2f),
    .diffuse = glm::vec3(0.5f, 0.5f, 0.5f),
    .specular = glm::vec3(1.0f, 1.0f, 1.0f),
  };

  glm::mat4 projection_;

  glm::vec2 last_mouse_;
};
