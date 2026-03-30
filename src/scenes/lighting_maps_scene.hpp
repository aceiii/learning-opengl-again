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


class LightingMapsScene final : public Scene {
public:
  void Init(IAppContext* ctx) override {
    ctx_ = ctx;

    textures_.push_back(LoadTexture(GL_TEXTURE0, "resources/textures/container2.png"));

    lighting_shader_ = Shader::FromFiles("resources/shaders/lighting_maps_scene/main.vs", "resources/shaders/lighting_maps_scene/main.fs");
    light_cube_shader_ = Shader::FromFiles("resources/shaders/lighting_maps_scene/light.vs", "resources/shaders/lighting_maps_scene/light.fs");

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
    lighting_shader_.SetInt("material.diffuse", 0);
    lighting_shader_.SetVec3("material.specular", material_.specular);
    lighting_shader_.SetFloat("material.shininess", material_.shininess);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures_[0]);
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

    ImGui::PushID("LightingMaps");
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
        if (ImGui::BeginCombo("Selected material", selected_material_idx_ < 0 ? "Custom" : kBuiltinMaterials[selected_material_idx_].first)) {
          for (int i = 0; i < kBuiltinMaterials.size(); i++) {
            const auto& mat = kBuiltinMaterials[i];
            if (ImGui::Selectable(mat.first, i == selected_material_idx_)) {
              material_ = mat.second;
              selected_material_idx_ = i;
            }
          }
          ImGui::EndCombo();
        }

        if (ImGui::ColorEdit3("Ambient", &material_.ambient[0])) {
          selected_material_idx_ = -1;
        }
        if (ImGui::ColorEdit3("Diffuse", &material_.diffuse[0])) {
          selected_material_idx_ = -1;
        }
        if (ImGui::ColorEdit3("Specular", &material_.specular[0])) {
          selected_material_idx_ = -1;
        }
        if (ImGui::DragFloat("Shininess", &material_.shininess, 0.01f, 0.01f, 10.0f)) {
          selected_material_idx_ = -1;
        }
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
    return "Lighting Maps";
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

  inline static const std::array kBuiltinMaterials{
    std::make_pair("Emerald", Material{ .ambient = { 0.0215f, 0.1745f, 0.0215f }, .diffuse = { 0.07568f, 0.61424f, 0.07568f }, .specular = { 0.633f, 0.727811f, 0.633f }, .shininess = 76.8f }),
    std::make_pair("Jade", Material{ .ambient = { 0.135f, 0.2225f, 0.1575f }, .diffuse = { 0.54f, 0.89f, 0.63f }, .specular = { 0.316228f, 0.316228f, 0.316228f }, .shininess = 12.8f }),
    std::make_pair("Obsidian", Material{ .ambient = { 0.05375f, 0.05f, 0.06625f }, .diffuse = { 0.18275f, 0.17f, 0.22525f }, .specular = { 0.332741f, 0.328634f, 0.346435f }, .shininess = 38.4f }),

    std::make_pair("Pearl", Material{ .ambient = { 0.25f, 0.20725f, 0.20725f }, .diffuse = { 1.0f, 0.829f, 0.829f }, .specular = { 0.296648f, 0.296648f, 0.296648f }, .shininess = 11.264f }),
    std::make_pair("Ruby", Material{ .ambient = { 0.1745f, 0.01175f, 0.01175f }, .diffuse = { 0.61424f, 0.04136f, 0.04136f }, .specular = { 0.727811f, 0.626959f, 0.626959f }, .shininess = 76.8f }),
    std::make_pair("Turquoise", Material{ .ambient = { 0.1f, 0.18725f, 0.1745f }, .diffuse = { 0.396f, 0.74151f, 0.69102f }, .specular = { 0.297254f, 0.30829f, 0.306678f }, .shininess = 12.8f }),

    std::make_pair("Brass", Material{ .ambient = { 0.329412f, 0.223529f, 0.027451f }, .diffuse = { 0.780392f, 0.568627f, 0.113725f }, .specular = { 0.992157f, 0.941176f, 0.807843f }, .shininess = 27.8974f }),
    std::make_pair("Bronze", Material{ .ambient = { 0.2125f, 0.1275f, 0.054f }, .diffuse = { 0.714f, 0.4284f, 0.18144f }, .specular = { 0.393548f, 0.271906f, 0.166721f }, .shininess = 25.6f }),
    std::make_pair("Chrome", Material{ .ambient = { 0.25f, 0.25f, 0.25f }, .diffuse = { 0.4f, 0.4f, 0.4f }, .specular = { 0.774597f, 0.774597f, 0.774597f }, .shininess = 76.8f }),
    std::make_pair("Copper", Material{ .ambient = { 0.19125f, 0.0735f, 0.0225f }, .diffuse = { 0.7038f, 0.27048f, 0.0828f }, .specular = { 0.256777f, 0.137622f, 0.086014f }, .shininess = 12.8f }),
    std::make_pair("Gold", Material{ .ambient = { 0.24725f, 0.1995f, 0.0745f }, .diffuse = { 0.75164f, 0.60648f, 0.22648f }, .specular = { 0.628281f, 0.555802f, 0.366065f }, .shininess = 51.2f }),
    std::make_pair("Silver", Material{ .ambient = { 0.19225f, 0.19225f, 0.19225f }, .diffuse = { 0.50754f, 0.50754f, 0.50754f }, .specular = { 0.508273f, 0.508273f, 0.508273f }, .shininess = 51.2f }),

    std::make_pair("Black Plastic", Material{ .ambient = { 0.0f, 0.0f, 0.0f }, .diffuse = { 0.01f, 0.01f, 0.01f }, .specular = { 0.50f, 0.50f, 0.50f }, .shininess = 32.0f }),
    std::make_pair("Cyan Plastic", Material{ .ambient = { 0.0f, 0.1f, 0.06f }, .diffuse = { 0.0f, 0.50980392f, 0.50980392f }, .specular = { 0.50196078f, 0.50196078f, 0.50196078f }, .shininess = 32.0f }),
    std::make_pair("Green Plastic", Material{ .ambient = { 0.0f, 0.0f, 0.0f }, .diffuse = { 0.1f, 0.35f, 0.1f }, .specular = { 0.45f, 0.55f, 0.45f }, .shininess = 32.0f }),
    std::make_pair("Red Plastic", Material{ .ambient = { 0.0f, 0.0f, 0.0f }, .diffuse = { 0.5f, 0.0f, 0.0f }, .specular = { 0.7f, 0.6f, 0.6f }, .shininess = 32.0f }),
    std::make_pair("White Plastic", Material{ .ambient = { 0.0f, 0.0f, 0.0f }, .diffuse = { 0.55f, 0.55f, 0.55f }, .specular = { 0.70f, 0.70f, 0.70f }, .shininess = 32.0f }),
    std::make_pair("Yellow Plastic", Material{ .ambient = { 0.0f, 0.0f, 0.0f }, .diffuse = { 0.5f, 0.5f, 0.0f }, .specular = { 0.60f, 0.60f, 0.50f }, .shininess = 32.0f }),

    std::make_pair("Black Rubber", Material{ .ambient = { 0.02f, 0.02f, 0.02f }, .diffuse = { 0.01f, 0.01f, 0.01f }, .specular = { 0.4f, 0.4f, 0.4f }, .shininess = 10.0f }),
    std::make_pair("Cyan Rubber", Material{ .ambient = { 0.0f, 0.05f, 0.05f }, .diffuse = { 0.4f, 0.5f, 0.5f }, .specular = { 0.04f, 0.7f, 0.7f }, .shininess = 10.0f }),
    std::make_pair("Green Rubber", Material{ .ambient = { 0.0f, 0.05f, 0.0f }, .diffuse = { 0.4f, 0.5f, 0.4f }, .specular = { 0.04f, 0.7f, 0.04f }, .shininess = 10.0f }),
    std::make_pair("Red Rubber", Material{ .ambient = { 0.05f, 0.0f, 0.0f }, .diffuse = { 0.5f, 0.4f, 0.4f }, .specular = { 0.7f, 0.04f, 0.04f }, .shininess = 10.0f }),
    std::make_pair("White Rubber", Material{ .ambient = { 0.05f, 0.05f, 0.05f }, .diffuse = { 0.5f, 0.5f, 0.5f }, .specular = { 0.7f, 0.7f, 0.7f }, .shininess = 10.0f }),
    std::make_pair("Yellow Rubber", Material{ .ambient = { 0.05f, 0.05f, 0.0f }, .diffuse = { 0.5f, 0.5f, 0.4f }, .specular = { 0.7f, 0.7f, 0.04f }, .shininess = 10.0f }),
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

  int selected_material_idx_ = -1;
};
