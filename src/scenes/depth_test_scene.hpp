#pragma once

#include <array>
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
#include "../model.hpp"


class DepthTestScene final : public Scene {
public:
  void Init(IAppContext* ctx) override {
    ctx_ = ctx;

    orig_bgcolor_ = ctx_->GetBackgroundColor();
    ctx_->SetBackgroundColor(environment_.bg_color);

    model_shader_ = Shader::FromFiles("resources/shaders/depth_test_scene/main.vs", "resources/shaders/depth_test_scene/main.fs");
    model_ = Model::Load("resources/models/backpack/backpack.obj");

    glEnable(GL_DEPTH_TEST);

    camera_.position = glm::vec3(0.82, 0.85f, 4.1f);
    camera_.yaw = -100.0f;
    camera_.pitch = -10.0f;
    camera_.UpdateCameraVectors();

    environment_.spot_light.position = camera_.position;
    environment_.spot_light.direction = camera_.front;
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

    if (flashlight_mode_) {
      environment_.spot_light.position = camera_.position;
      environment_.spot_light.direction = camera_.front;
    }
  }

  void Render() override {
    glPolygonMode(GL_FRONT_AND_BACK, wireframe_ ? GL_LINE : GL_FILL);

    glm::mat4 view = camera_.GetViewMatrix();

    model_shader_.Use();
    model_shader_.SetMat4("view", view);
    model_shader_.SetMat4("projection", projection_);
    model_shader_.SetVec3("viewPos", camera_.position);

    model_shader_.SetVec3("directionalLight.direction", environment_.directional_light.direction);
    model_shader_.SetVec3("directionalLight.ambient", environment_.directional_light.ambient);
    model_shader_.SetVec3("directionalLight.diffuse", environment_.directional_light.diffuse);
    model_shader_.SetVec3("directionalLight.specular", environment_.directional_light.specular);

    model_shader_.SetVec3("spotLight.position", environment_.spot_light.position);
    model_shader_.SetVec3("spotLight.direction", environment_.spot_light.direction);
    model_shader_.SetVec3("spotLight.ambient", environment_.spot_light.ambient);
    model_shader_.SetVec3("spotLight.diffuse", environment_.spot_light.diffuse);
    model_shader_.SetVec3("spotLight.specular", environment_.spot_light.specular);
    model_shader_.SetFloat("spotLight.cutOff", environment_.spot_light.cutOff);
    model_shader_.SetFloat("spotLight.outerCutOff", environment_.spot_light.outerCutOff);
    model_shader_.SetFloat("spotLight.constant", environment_.spot_light.constant);
    model_shader_.SetFloat("spotLight.linear", environment_.spot_light.linear);
    model_shader_.SetFloat("spotLight.quadratic", environment_.spot_light.quadratic);

    for (auto i = 0; i < environment_.point_lights.size(); i++) {
      const auto& light = environment_.point_lights[i];
      model_shader_.SetVec3(std::format("pointLight[{}].position", i), light.position);
      model_shader_.SetVec3(std::format("pointLight[{}].ambient", i), light.ambient);
      model_shader_.SetVec3(std::format("pointLight[{}].diffuse", i), light.diffuse);
      model_shader_.SetVec3(std::format("pointLight[{}].specular", i), light.specular);
      model_shader_.SetFloat(std::format("pointLight[{}].constant", i), light.constant);
      model_shader_.SetFloat(std::format("pointLight[{}].linear", i), light.linear);
      model_shader_.SetFloat(std::format("pointLight[{}].quadratic", i), light.quadratic);
    }

    // model_shader_.SetInt("material.diffuse", material_.diffuse);
    // model_shader_.SetInt("material.specular", material_.specular);
    model_shader_.SetFloat("shininess", shininess_);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
    model_shader_.SetMat4("model", model);

    model_.Draw(model_shader_);
  }

  void RenderInterface(int window_width, int window_height) override {
    constexpr auto padding = 5.0f;
    constexpr auto menu_bar_height = 32.0f;

    ImGui::PushID("DepthTest");
    ImGui::SetNextWindowPos(ImVec2(window_width - padding, menu_bar_height - padding), ImGuiCond_FirstUseEver, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Scene Options")) {
      ImGui::Checkbox("Wireframe", &wireframe_);
      ImGui::NewLine();

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
    if (ctx_) {
      ctx_->SetBackgroundColor(orig_bgcolor_);
      ctx_ = nullptr;
    }

    model_shader_.Destroy();
  }

  std::string Name() const override {
    return "Depth Test";
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

  struct DirectionalLight {
      glm::vec3 direction;
      glm::vec3 ambient;
      glm::vec3 diffuse;
      glm::vec3 specular;
  };

  struct PointLight {
      glm::vec3 position;
      glm::vec3 ambient;
      glm::vec3 diffuse;
      glm::vec3 specular;
      float constant;
      float linear;
      float quadratic;
  };

  struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float constant;
    float linear;
    float quadratic;
    float cutOff;
    float outerCutOff;
  };

  struct Environment {
    std::string name;
    glm::vec3 bg_color;
    DirectionalLight directional_light;
    SpotLight spot_light;
    std::array<PointLight, 4> point_lights;
  };

  IAppContext* ctx_ = nullptr;

  inline static const float kDefaultYaw = -90.0f;
  inline static const float kDefaultPitch = 0.0f;
  inline static const float kMinPitch = -89.0f;
  inline static const float kMaxPitch = 89.0f;

  Shader model_shader_;
  Camera camera_{glm::vec3(0.0f, 0.0f, 3.0f)};
  Environment environment_{
      .name = "Default",
      .bg_color = glm::vec3(0.5, 0.5, 0.5),
      .directional_light = {
        .direction = glm::vec3(-0.2f, -1.0f, -0.3f),
        .ambient = glm::vec3(0.05f, 0.05f, 0.05f),
        .diffuse = glm::vec3(0.4f, 0.4f, 0.4f),
        .specular = glm::vec3(0.5f, 0.5f, 0.5f),
      },
      .spot_light = {
        .position = glm::vec3(0.0f, 0.0f, 0.0f),
        .direction = glm::vec3(0.0f, 0.0f, 0.0f),
        .ambient = glm::vec3(0.0f, 0.0f, 0.0f),
        .diffuse = glm::vec3(1.0f, 1.0f, 1.0f),
        .specular = glm::vec3(1.0f, 1.0f, 1.0f),
        .constant = 1.0f,
        .linear = 0.09f,
        .quadratic = 0.032f,
        .cutOff = glm::cos(glm::radians(12.5f)),
        .outerCutOff = glm::cos(glm::radians(15.0f)),
      },
      .point_lights = {
        PointLight{
          .position = glm::vec3(0.7f,  0.2f,  2.0f),
          .ambient = glm::vec3(0.05f, 0.05f, 0.05f),
          .diffuse = glm::vec3(0.8f, 0.8f, 0.8f),
          .specular = glm::vec3(1.0f, 1.0f, 1.0f),
          .constant = 1.0f,
          .linear = 0.09f,
          .quadratic = 0.032f,
        },
        PointLight{
          .position = glm::vec3(2.3f, -3.3f, -4.0f),
          .ambient = glm::vec3(0.05f, 0.05f, 0.05f),
          .diffuse = glm::vec3(0.8f, 0.8f, 0.8f),
          .specular = glm::vec3(1.0f, 1.0f, 1.0f),
          .constant = 1.0f,
          .linear = 0.09f,
          .quadratic = 0.032f,
        },
        PointLight{
          .position = glm::vec3(-4.0f,  2.0f, -12.0f),
          .ambient = glm::vec3(0.05f, 0.05f, 0.05f),
          .diffuse = glm::vec3(0.8f, 0.8f, 0.8f),
          .specular = glm::vec3(1.0f, 1.0f, 1.0f),
          .constant = 1.0f,
          .linear = 0.09f,
          .quadratic = 0.032f,
        },
        PointLight{
          .position = glm::vec3(0.0f,  0.0f, -3.0f),
          .ambient = glm::vec3(0.05f, 0.05f, 0.05f),
          .diffuse = glm::vec3(0.8f, 0.8f, 0.8f),
          .specular = glm::vec3(1.0f, 1.0f, 1.0f),
          .constant = 1.0f,
          .linear = 0.09f,
          .quadratic = 0.032f,
        },
      },
    };

  Model model_;

  int selected_environment_ = 0;

  glm::mat4 projection_;
  glm::vec3 orig_bgcolor_;
  glm::vec2 last_mouse_;

  bool wireframe_ = false;
  bool auto_rotate_camera_ = false;
  bool capture_mouse_ = false;
  bool capture_hold_ = false;
  bool reset_mouse_ = true;
  bool hide_interface_ = true;
  bool flashlight_mode_ = true;

  float aspect_ratio_ = 800.0f / 600.0f;
  float camera_radius_ = 10.0f;
  float shininess_ = 32.0f;

};
