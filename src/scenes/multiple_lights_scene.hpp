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


class MultipleLightsScene final : public Scene {
public:
  void Init(IAppContext* ctx) override {
    ctx_ = ctx;

    orig_bgcolor_ = ctx_->GetBackgroundColor();
    ctx_->SetBackgroundColor(environment_.bg_color);

    textures_.push_back(LoadTexture(GL_TEXTURE0, "resources/textures/container2.png"));
    textures_.push_back(LoadTexture(GL_TEXTURE1, "resources/textures/container2_specular.png"));
    textures_.push_back(LoadTexture(GL_TEXTURE2, "resources/textures/lighting_maps_specular_color.png"));
    textures_.push_back(LoadTexture(GL_TEXTURE3, "resources/textures/matrix.jpg"));

    lighting_shader_ = Shader::FromFiles("resources/shaders/multiple_lights_scene/main.vs", "resources/shaders/multiple_lights_scene/main.fs");
    light_cube_shader_ = Shader::FromFiles("resources/shaders/multiple_lights_scene/light.vs", "resources/shaders/multiple_lights_scene/light.fs");

    vaos_.resize(2);
    vbos_.resize(1);

    glGenVertexArrays(vaos_.size(), vaos_.data());
    glGenBuffers(vbos_.size(), vbos_.data());

    glBindVertexArray(vaos_[0]);
    glBindBuffer(GL_ARRAY_BUFFER, vbos_[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(vaos_[1]);
    glBindBuffer(GL_ARRAY_BUFFER, vbos_[0]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
    glEnableVertexAttribArray(0);

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

    lighting_shader_.Use();
    lighting_shader_.SetMat4("view", view);
    lighting_shader_.SetMat4("projection", projection_);
    lighting_shader_.SetVec3("viewPos", camera_.position);

    lighting_shader_.SetVec3("directionalLight.direction", environment_.directional_light.direction);
    lighting_shader_.SetVec3("directionalLight.ambient", environment_.directional_light.ambient);
    lighting_shader_.SetVec3("directionalLight.diffuse", environment_.directional_light.diffuse);
    lighting_shader_.SetVec3("directionalLight.specular", environment_.directional_light.specular);

    lighting_shader_.SetVec3("spotLight.position", environment_.spot_light.position);
    lighting_shader_.SetVec3("spotLight.direction", environment_.spot_light.direction);
    lighting_shader_.SetVec3("spotLight.ambient", environment_.spot_light.ambient);
    lighting_shader_.SetVec3("spotLight.diffuse", environment_.spot_light.diffuse);
    lighting_shader_.SetVec3("spotLight.specular", environment_.spot_light.specular);
    lighting_shader_.SetFloat("spotLight.cutOff", environment_.spot_light.cutOff);
    lighting_shader_.SetFloat("spotLight.outerCutOff", environment_.spot_light.outerCutOff);
    lighting_shader_.SetFloat("spotLight.constant", environment_.spot_light.constant);
    lighting_shader_.SetFloat("spotLight.linear", environment_.spot_light.linear);
    lighting_shader_.SetFloat("spotLight.quadratic", environment_.spot_light.quadratic);

    for (auto i = 0; i < environment_.point_lights.size(); i++) {
      const auto& light = environment_.point_lights[i];
      lighting_shader_.SetVec3(std::format("pointLight[{}].position", i), light.position);
      lighting_shader_.SetVec3(std::format("pointLight[{}].ambient", i), light.ambient);
      lighting_shader_.SetVec3(std::format("pointLight[{}].diffuse", i), light.diffuse);
      lighting_shader_.SetVec3(std::format("pointLight[{}].specular", i), light.specular);
      lighting_shader_.SetFloat(std::format("pointLight[{}].constant", i), light.constant);
      lighting_shader_.SetFloat(std::format("pointLight[{}].linear", i), light.linear);
      lighting_shader_.SetFloat(std::format("pointLight[{}].quadratic", i), light.quadratic);
    }

    lighting_shader_.SetInt("material.diffuse", material_.diffuse);
    lighting_shader_.SetInt("material.specular", material_.specular);
    lighting_shader_.SetFloat("material.shininess", material_.shininess);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures_[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textures_[1]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, textures_[2]);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, textures_[3]);
    glBindVertexArray(vaos_[0]);

    for (int i = 0; i < 10; i++) {
      float angle = 20.0f * i;

      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, kCubePositions[i]);
      model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));

      lighting_shader_.SetMat4("model", model);
      glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    for (const auto& light : environment_.point_lights) {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, light.position);
      model = glm::scale(model, glm::vec3(0.1f));

      light_cube_shader_.Use();
      light_cube_shader_.SetMat4("view", view);
      light_cube_shader_.SetMat4("projection", projection_);
      light_cube_shader_.SetMat4("model", model);
      light_cube_shader_.SetVec3("lightColor", light.diffuse);

      glBindVertexArray(vaos_[0]);
      glDrawArrays(GL_TRIANGLES, 0, 36);
    }
  }

  void RenderInterface(int window_width, int window_height) override {
    constexpr auto padding = 5.0f;
    constexpr auto menu_bar_height = 32.0f;

    ImGui::PushID("MultipleLights");
    ImGui::SetNextWindowPos(ImVec2(window_width - padding, menu_bar_height - padding), ImGuiCond_FirstUseEver, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Scene Options")) {
      ImGui::Checkbox("Wireframe", &wireframe_);
      ImGui::NewLine();
      if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Lighting");
        if (ImGui::BeginCombo("Environment", environment_.name.data())) {
          for (int idx = 0; idx < kEnvironments.size(); idx++) {
            const auto& env = kEnvironments[idx];
            if (ImGui::Selectable(env.name.data(), selected_environment_ == idx)) {
              SpotLight prev_spot_light = environment_.spot_light;
              environment_ = env;
              environment_.spot_light.position = prev_spot_light.position;
              environment_.spot_light.direction = prev_spot_light.direction;
              ctx_->SetBackgroundColor(environment_.bg_color);
              selected_environment_ = idx;
            }
          }
          ImGui::EndCombo();
        }
        ImGui::Checkbox("Flashlight mode", &flashlight_mode_);
        ImGui::PopID();
      }
      if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Material");
        ImGui::DragInt("Diffuse", &material_.diffuse, 1, 0, 8);
        ImGui::DragInt("Specular", &material_.specular, 1, 0, 8);
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
    if (ctx_) {
      ctx_->SetBackgroundColor(orig_bgcolor_);
      ctx_ = nullptr;
    }

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
    return "Multiple Lights";
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

  struct Material {
    int diffuse;
    int specular;
    float shininess;
  };

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

  inline static const std::array kVertices{
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
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

  struct Environment {
    std::string name;
    glm::vec3 bg_color;
    DirectionalLight directional_light;
    SpotLight spot_light;
    std::array<PointLight, 4> point_lights;
  };

  inline static const std::array kEnvironments{
    Environment{
      .name = "Default",
      .bg_color = glm::vec3(0.074509f, 0.070588f, 0.078431f),
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
    },
    Environment{
      .name = "Desert",
      .bg_color = glm::vec3(0.75f, 0.52f, 0.3f),
      .directional_light = {
        .direction = glm::vec3(-0.2f, -1.0f, -0.3f),
        .ambient = glm::vec3(0.3f, 0.24f, 0.14f),
        .diffuse = glm::vec3(0.7f, 0.42f, 0.26f),
        .specular = glm::vec3(0.5f, 0.5f, 0.5f),
      },
      .spot_light = {
        .position = glm::vec3(0.0f, 0.0f, 0.0f),
        .direction = glm::vec3(0.0f, 0.0f, 0.0f),
        .ambient = glm::vec3(0.0f, 0.0f, 0.0f),
        .diffuse = glm::vec3(0.8f, 0.8f, 0.0f),
        .specular = glm::vec3(0.8f, 0.8f, 0.0f),
        .constant = 1.0f,
        .linear = 0.09f,
        .quadratic = 0.032f,
        .cutOff = glm::cos(glm::radians(12.5f)),
        .outerCutOff = glm::cos(glm::radians(15.0f)),
      },
      .point_lights = {
        PointLight{
          .position = glm::vec3(0.7f,  0.2f,  2.0f),
          .ambient = glm::vec3(0.1f, 0.06f, 0.0f),
          .diffuse = glm::vec3(1.0f, 0.6f, 0.0f),
          .specular = glm::vec3(1.0f, 0.6f, 0.0f),
          .constant = 1.0f,
          .linear = 0.09f,
          .quadratic = 0.032f,
        },
        PointLight{
          .position = glm::vec3(2.3f, -3.3f, -4.0f),
          .ambient = glm::vec3(0.1f, 0.0f, 0.0f),
          .diffuse = glm::vec3(1.0f, 0.0f, 0.0f),
          .specular = glm::vec3(1.0f, 0.0f, 0.0f),
          .constant = 1.0f,
          .linear = 0.09f,
          .quadratic = 0.032f,
        },
        PointLight{
          .position = glm::vec3(-4.0f,  2.0f, -12.0f),
          .ambient = glm::vec3(0.1f, 0.1f, 0.0f),
          .diffuse = glm::vec3(1.0f, 1.0f, 0.0f),
          .specular = glm::vec3(1.0f, 1.0f, 0.0f),
          .constant = 1.0f,
          .linear = 0.09f,
          .quadratic = 0.032f,
        },
        PointLight{
          .position = glm::vec3(0.0f,  0.0f, -3.0f),
          .ambient = glm::vec3(0.02f, 0.02f, 0.1f),
          .diffuse = glm::vec3(0.2f, 0.2f, 1.0f),
          .specular = glm::vec3(0.2f, 0.2f, 1.0f),
          .constant = 1.0f,
          .linear = 0.09f,
          .quadratic = 0.032f,
        },
      },
    },
    Environment{
      .name = "Factory",
      .bg_color = glm::vec3(0.1f, 0.1f, 0.1f),
      .directional_light = {
        .direction = glm::vec3(-0.2f, -1.0f, -0.3f),
        .ambient = glm::vec3(0.05f, 0.05f, 0.1f),
        .diffuse = glm::vec3(0.2f, 0.2f, 0.7f),
        .specular = glm::vec3(0.7f, 0.7f, 0.7f),
      },
      .spot_light = {
        .position = glm::vec3(0.0f, 0.0f, 0.0f),
        .direction = glm::vec3(0.0f, 0.0f, 0.0f),
        .ambient = glm::vec3(0.0f, 0.0f, 0.0f),
        .diffuse = glm::vec3(1.0f, 1.0f, 1.0f),
        .specular = glm::vec3(1.0f, 1.0f, 1.0f),
        .constant = 1.0f,
        .linear = 0.009f,
        .quadratic = 0.0032f,
        .cutOff = glm::cos(glm::radians(10.5f)),
        .outerCutOff = glm::cos(glm::radians(12.5f)),
      },
      .point_lights = {
        PointLight{
          .position = glm::vec3(0.7f,  0.2f,  2.0f),
          .ambient = glm::vec3(0.02f, 0.02f, 0.06f),
          .diffuse = glm::vec3(0.2f, 0.2f, 0.6f),
          .specular = glm::vec3(0.2f, 0.2f, 0.6f),
          .constant = 1.0f,
          .linear = 0.09f,
          .quadratic = 0.032f,
        },
        PointLight{
          .position = glm::vec3(2.3f, -3.3f, -4.0f),
          .ambient = glm::vec3(0.03f, 0.03f, 0.07f),
          .diffuse = glm::vec3(0.3f, 0.3f, 0.7f),
          .specular = glm::vec3(0.3f, 0.3f, 0.7f),
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
    },
    Environment{
      .name = "Horror",
      .bg_color = glm::vec3(0.0f, 0.0f, 0.0f),
      .directional_light = {
        .direction = glm::vec3(-0.2f, -1.0f, -0.3f),
        .ambient = glm::vec3(0.0f, 0.0f, 0.0f),
        .diffuse = glm::vec3(0.05f, 0.05f, 0.05f),
        .specular = glm::vec3(0.2f, 0.2f, 0.2f),
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
        .cutOff = glm::cos(glm::radians(10.0f)),
        .outerCutOff = glm::cos(glm::radians(15.0f)),
      },
      .point_lights = {
        PointLight{
          .position = glm::vec3(0.7f,  0.2f,  2.0f),
          .ambient = glm::vec3(0.01f, 0.01f, 0.01f),
          .diffuse = glm::vec3(0.1f, 0.1f, 0.1f),
          .specular = glm::vec3(0.1f, 0.1f, 0.1f),
          .constant = 1.0f,
          .linear = 0.14f,
          .quadratic = 0.07f,
        },
        PointLight{
          .position = glm::vec3(2.3f, -3.3f, -4.0f),
          .ambient = glm::vec3(0.01f, 0.01f, 0.01f),
          .diffuse = glm::vec3(0.1f, 0.1f, 0.1f),
          .specular = glm::vec3(0.1f, 0.1f, 0.1f),
          .constant = 1.0f,
          .linear = 0.14f,
          .quadratic = 0.07,
        },
        PointLight{
          .position = glm::vec3(-4.0f,  2.0f, -12.0f),
          .ambient = glm::vec3(0.01f, 0.01f, 0.01f),
          .diffuse = glm::vec3(0.1f, 0.1f, 0.1f),
          .specular = glm::vec3(0.1f, 0.1f, 0.1f),
          .constant = 1.0f,
          .linear = 0.22f,
          .quadratic = 0.2f,
        },
        PointLight{
          .position = glm::vec3(0.0f,  0.0f, -3.0f),
          .ambient = glm::vec3(0.03f, 0.01f, 0.01f),
          .diffuse = glm::vec3(0.3f, 0.1f, 0.1f),
          .specular = glm::vec3(0.3f, 0.1f, 0.1f),
          .constant = 1.0f,
          .linear = 0.14f,
          .quadratic = 0.07f,
        },
      },
    },
    Environment{
      .name = "Biochemical Lab",
      .bg_color = glm::vec3(0.9f, 0.9f, 0.9f),
      .directional_light = {
        .direction = glm::vec3(-0.2f, -1.0f, -0.3f),
        .ambient = glm::vec3(0.5f, 0.5f, 0.5f),
        .diffuse = glm::vec3(1.0f, 1.0f, 1.0f),
        .specular = glm::vec3(1.0f, 1.0f, 1.0f),
      },
      .spot_light = {
        .position = glm::vec3(0.0f, 0.0f, 0.0f),
        .direction = glm::vec3(0.0f, 0.0f, 0.0f),
        .ambient = glm::vec3(0.0f, 0.0f, 0.0f),
        .diffuse = glm::vec3(0.0f, 1.0f, 0.0f),
        .specular = glm::vec3(0.0f, 1.0f, 0.0f),
        .constant = 1.0f,
        .linear = 0.07f,
        .quadratic = 0.017f,
        .cutOff = glm::cos(glm::radians(7.f)),
        .outerCutOff = glm::cos(glm::radians(10.0f)),
      },
      .point_lights = {
        PointLight{
          .position = glm::vec3(0.7f,  0.2f,  2.0f),
          .ambient = glm::vec3(0.04f, 0.07f, 0.01f),
          .diffuse = glm::vec3(0.4f, 0.7f, 0.1f),
          .specular = glm::vec3(0.4f, 0.7f, 0.1f),
          .constant = 1.0f,
          .linear = 0.07f,
          .quadratic = 0.017f,
        },
        PointLight{
          .position = glm::vec3(2.3f, -3.3f, -4.0f),
          .ambient = glm::vec3(0.04f, 0.07f, 0.01f),
          .diffuse = glm::vec3(0.4f, 0.7f, 0.1f),
          .specular = glm::vec3(0.4f, 0.7f, 0.1f),
          .constant = 1.0f,
          .linear = 0.07f,
          .quadratic = 0.017f,
        },
        PointLight{
          .position = glm::vec3(-4.0f,  2.0f, -12.0f),
          .ambient = glm::vec3(0.04f, 0.07f, 0.01f),
          .diffuse = glm::vec3(0.4f, 0.7f, 0.1f),
          .specular = glm::vec3(0.4f, 0.7f, 0.1f),
          .constant = 1.0f,
          .linear = 0.07f,
          .quadratic = 0.017f,
        },
        PointLight{
          .position = glm::vec3(0.0f,  0.0f, -3.0f),
          .ambient = glm::vec3(0.04f, 0.07f, 0.01f),
          .diffuse = glm::vec3(0.4f, 0.7f, 0.1f),
          .specular = glm::vec3(0.4f, 0.7f, 0.1f),
          .constant = 1.0f,
          .linear = 0.07f,
          .quadratic = 0.017f,
        },
      },
    },
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
  std::vector<unsigned int> textures_;

  bool wireframe_ = false;
  bool auto_rotate_camera_ = false;
  bool capture_mouse_ = false;
  bool capture_hold_ = false;
  bool reset_mouse_ = true;
  bool hide_interface_ = true;
  bool flashlight_mode_ = true;

  float aspect_ratio_ = 800.0f / 600.0f;
  float camera_radius_ = 10.0f;

  Camera camera_{glm::vec3(0.0f, 0.0f, 3.0f)};
  Material material_{
    .diffuse = 0,
    .specular = 1,
    .shininess = 32.0f,
  };

  Environment environment_ = kEnvironments[0];
  int selected_environment_ = 0;

  glm::mat4 projection_;
  glm::vec3 orig_bgcolor_;
  glm::vec2 last_mouse_;
};
