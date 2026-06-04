#pragma once

#include <array>
#include <map>
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
#include "../texture.hpp"
#include "../random.hpp"
#include "../util.hpp"

using namespace std::string_literals;

class SSAOScene final : public Scene {
public:
  static constexpr unsigned int kShadowWidth = 1024;
  static constexpr unsigned int kShadowHeight = 1024;

  void Init(IAppContext* ctx) override {
    ctx_ = ctx;

    orig_bgcolor_ = ctx_->GetBackgroundColor();
    ctx_->SetBackgroundColor(bg_color_);

    shader_ = Shader::FromFiles("resources/shaders/ssao_scene/geom.vs", "resources/shaders/ssao_scene/geom.fs");
    ssao_shader_ = Shader::FromFiles("resources/shaders/ssao_scene/ssao.vs", "resources/shaders/ssao_scene/ssao.fs");
    lighting_shader_ = Shader::FromFiles("resources/shaders/ssao_scene/lighting.vs", "resources/shaders/ssao_scene/lighting.fs");
    light_shader_ = Shader::FromFiles("resources/shaders/ssao_scene/light.vs", "resources/shaders/ssao_scene/light.fs");

    projection_ = glm::perspective(glm::radians(camera_.fov), aspect_ratio_, 0.1f, 100.0f);
    camera_.position = glm::vec3(9.5f, 0.5f, 5.0f);
    camera_.yaw = -148.0f;
    camera_.pitch = -4.5f;
    camera_.UpdateCameraVectors();

    auto rand = Random::RandFloat();

    const auto num_lights = 32;
    for (auto idx = 0; idx < num_lights; idx++) {
      float x = rand.Next() * 6.0 - 3.0;
      float y = rand.Next() * 6.0 - 4.0;
      float z = rand.Next() * 6.0 - 3.0;
      float r = (rand.Next() / 2.0) + 0.5f;
      float g = (rand.Next() / 2.0) + 0.5f;
      float b = (rand.Next() / 2.0) + 0.5f;

      float constant = 1.0f;
      float linear = 0.7f;
      float quadratic = 1.8f;
      float light_max = std::fmaxf(std::fmaxf(r, b), b);
      float radius = (-linear +  std::sqrtf(linear * linear - 4 * quadratic * (constant - (256.0f / 5.0f) * light_max))) / (2 * quadratic);

      Light light{
        .position = glm::vec3(x, y, z),
        .color = glm::vec3(r, g, b),
        .radius = radius,
      };
      lights_.push_back(light);

      LogInfo("Adding light at pos=({}, {}, {}), color=({}, {}, {})",
        light.position.x, light.position.y, light.position.z,
        light.color.r, light.color.g, light.color.b);
    }

    for (auto idx = 0; idx < 64; idx++) {
      glm::vec3 sample(
        rand.Next() * 2.0 - 1.0,
        rand.Next() * 2.0 - 1.0,
        rand.Next()
      );
      sample = glm::normalize(sample);
      sample *= rand.Next();

      float scale = static_cast<float>(idx) / 64.0f;
      scale = Util::Lerp(0.1f, 1.0f, scale * scale);
      sample *= scale;
      ssao_kernel_.push_back(sample);
    }

    std::array quad_vertices{
      -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
      -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
       1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
       1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };

    glGenVertexArrays(1, &quad_vao_);
    glGenBuffers(1, &quad_vbo_);
    glBindVertexArray(quad_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    auto [frame_width, frame_height] = ctx_->GetFramebufferSize();

    glGenFramebuffers(1, &g_buffer_);
    glBindFramebuffer(GL_FRAMEBUFFER, g_buffer_);

    glGenTextures(1, &g_position_);
    glBindTexture(GL_TEXTURE_2D, g_position_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, frame_width, frame_height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_position_, 0);

    glGenTextures(1, &g_normal_);
    glBindTexture(GL_TEXTURE_2D, g_normal_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, frame_width, frame_height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, g_normal_, 0);

    glGenTextures(1, &g_albedo_spec_);
    glBindTexture(GL_TEXTURE_2D, g_albedo_spec_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, frame_width, frame_height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, g_albedo_spec_, 0);

    glGenRenderbuffers(1, &depth_rbo_);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, frame_width, frame_width);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rbo_);

    std::array<GLenum, 3> attachments{ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(attachments.size(), attachments.data());

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      LogWarning("Framebuffer not complete: g_buffer_");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    std::vector<glm::vec3> ssao_noise;
    for (auto idx = 0; idx < 16; idx++) {
      glm::vec3 noise(
        rand.Next() * 2.0 - 1.0,
        rand.Next() * 2.0 - 1.0,
        0.0f
      );

      ssao_noise.push_back(noise);
    }

    glGenTextures(1, &noise_texture_);
    glBindTexture(GL_TEXTURE_2D, noise_texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, ssao_noise.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenFramebuffers(1, &ssao_fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, ssao_fbo_);
    glGenTextures(1, &ssao_color_buffer_);
    glBindTexture(GL_TEXTURE_2D, ssao_color_buffer_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame_width, frame_height, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssao_color_buffer_, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
  }

  void RenderQuad() {
    glBindVertexArray(quad_vao_);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
  }

  void RenderScene() {
    glPolygonMode(GL_FRONT_AND_BACK,  wireframe_ ? GL_LINE : GL_FILL);
    glm::mat4 model;

    shader_.Use();
    shader_.SetMat4("view", camera_.GetViewMatrix());
    shader_.SetMat4("projection", projection_);
    shader_.SetVec3("viewPos", camera_.position);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 7.0f, 0.0f));
    model = glm::scale(model, glm::vec3(7.5f));
    shader_.SetMat4("model", model);
    shader_.SetBool("inverseNormals", true);
    cube_mesh_.Draw(shader_);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.5f, 0.0f));
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(0.5f));
    shader_.SetMat4("model", model);
    shader_.SetBool("inverseNormals", false);
    backpack_.Draw(shader_);
  }

  void Render() override {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glBindFramebuffer(GL_FRAMEBUFFER, g_buffer_);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    RenderScene();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


    glBindFramebuffer(GL_FRAMEBUFFER, ssao_fbo_);
    glClear(GL_COLOR_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_position_);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_normal_);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, g_albedo_spec_);

    ssao_shader_.Use();
    ssao_shader_.SetInt("texture_position", 0);
    ssao_shader_.SetInt("texture_normal", 1);
    ssao_shader_.SetInt("texture_albedo_spec", 2);
    ssao_shader_.SetInt("debug", enable_debug_ ? debug_mode_ : 0);
    ssao_shader_.SetVec3("viewPos", camera_.position);
    ssao_shader_.SetMat4("projection", projection_);
    for (auto idx = 0; idx < ssao_kernel_.size(); idx++) {
      ssao_shader_.SetVec3("samples[" + std::to_string(idx) + "]", ssao_kernel_.data());
    }
    RenderQuad();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, ssao_color_buffer_);
    lighting_shader_.Use();
    lighting_shader_.SetInt("texture_position", 0);
    lighting_shader_.SetInt("texture_normal", 1);
    lighting_shader_.SetInt("texture_albedo_spec", 2);
    lighting_shader_.SetInt("texture_ssao", 3);
    lighting_shader_.SetInt("debug", enable_debug_ ? debug_mode_ : 0);
    lighting_shader_.SetVec3("viewPos", camera_.position);
    lighting_shader_.SetMat4("projection", projection_);
    RenderQuad();
  }

  void RenderInterface(int window_width, int window_height) override {
    constexpr auto padding = 5.0f;
    constexpr auto menu_bar_height = 32.0f;

    ImGui::PushID("SSAO");
    ImGui::SetNextWindowPos(ImVec2(window_width - padding, menu_bar_height - padding), ImGuiCond_FirstUseEver, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Scene Options")) {
      ImGui::Checkbox("Wireframe", &wireframe_);
      ImGui::NewLine();
      ImGui::Checkbox("Debug", &enable_debug_);
      if (enable_debug_) {
        if (ImGui::BeginCombo("Debug Mode", kDebugModes[debug_mode_-1].c_str())) {
          for (auto idx = 0; idx < kDebugModes.size(); idx++) {
            if (ImGui::Selectable(kDebugModes[idx].c_str(), idx + 1 == debug_mode_)) {
              debug_mode_ = idx + 1;
            }
          }
          ImGui::EndCombo();
        }
      }
      ImGui::NewLine();

      if (ImGui::CollapsingHeader("Camera")) {
        ImGui::Checkbox("Hide UI During Capture", &hide_interface_);
        ImGui::Checkbox("Hold Capture on mouse press", &capture_hold_);
        // ImGui::DragFloat("Field of View", &camera_.fov, 0.1f, Camera::kMinFov, Camera::kMaxFov);
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

    shader_.Destroy();
  }

  std::string Name() const override {
    return "SSAO";
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

  struct Light {
    glm::vec3 position;
    glm::vec3 color;
    float radius;
  };

  IAppContext* ctx_ = nullptr;

  inline static const float kDefaultYaw = -90.0f;
  inline static const float kDefaultPitch = 0.0f;
  inline static const float kMinPitch = -89.0f;
  inline static const float kMaxPitch = 89.0f;

  inline static const std::array kDebugModes{
    "Position"s,
    "Normal"s,
    "Albedo"s,
    "Specular"s,
  };

  inline static const std::array kObjectPositions{
    glm::vec3(-3.0,  -0.5, -3.0),
    glm::vec3( 0.0,  -0.5, -3.0),
    glm::vec3( 3.0,  -0.5, -3.0),
    glm::vec3(-3.0,  -0.5,  0.0),
    glm::vec3( 0.0,  -0.5,  0.0),
    glm::vec3( 3.0,  -0.5,  0.0),
    glm::vec3(-3.0,  -0.5,  3.0),
    glm::vec3( 0.0,  -0.5,  3.0),
    glm::vec3( 3.0,  -0.5,  3.0),
  };

  Shader shader_;
  Shader lighting_shader_;
  Shader ssao_shader_;
  Shader light_shader_;

  Model backpack_ = Model::Load("resources/models/backpack/backpack.obj", { .texture_options = { .linear = true }});

  Mesh cube_mesh_{
    {
      { { -1.0f, -1.0f, -1.0f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 0.0f } },
      { {  1.0f,  1.0f, -1.0f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f } },
      { {  1.0f, -1.0f, -1.0f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 0.0f } },
      { {  1.0f,  1.0f, -1.0f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f } },
      { { -1.0f, -1.0f, -1.0f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 0.0f } },
      { { -1.0f,  1.0f, -1.0f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 1.0f } },
      { { -1.0f, -1.0f,  1.0f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 0.0f } },
      { {  1.0f, -1.0f,  1.0f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 0.0f } },
      { {  1.0f,  1.0f,  1.0f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 1.0f } },
      { {  1.0f,  1.0f,  1.0f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 1.0f } },
      { { -1.0f,  1.0f,  1.0f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 1.0f } },
      { { -1.0f, -1.0f,  1.0f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 0.0f } },
      { { -1.0f,  1.0f,  1.0f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f } },
      { { -1.0f,  1.0f, -1.0f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } },
      { { -1.0f, -1.0f, -1.0f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } },
      { { -1.0f, -1.0f, -1.0f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } },
      { { -1.0f, -1.0f,  1.0f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } },
      { { -1.0f,  1.0f,  1.0f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f } },
      { {  1.0f,  1.0f,  1.0f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f } },
      { {  1.0f, -1.0f, -1.0f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } },
      { {  1.0f,  1.0f, -1.0f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } },
      { {  1.0f, -1.0f, -1.0f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } },
      { {  1.0f,  1.0f,  1.0f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f } },
      { {  1.0f, -1.0f,  1.0f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } },
      { { -1.0f, -1.0f, -1.0f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 1.0f } },
      { {  1.0f, -1.0f, -1.0f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 1.0f } },
      { {  1.0f, -1.0f,  1.0f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 0.0f } },
      { {  1.0f, -1.0f,  1.0f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 0.0f } },
      { { -1.0f, -1.0f,  1.0f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 0.0f } },
      { { -1.0f, -1.0f, -1.0f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 1.0f } },
      { { -1.0f,  1.0f, -1.0f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 1.0f } },
      { {  1.0f,  1.0f , 1.0f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 0.0f } },
      { {  1.0f,  1.0f, -1.0f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f } },
      { {  1.0f,  1.0f,  1.0f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 0.0f } },
      { { -1.0f,  1.0f, -1.0f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 1.0f } },
      { { -1.0f,  1.0f,  1.0f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 0.0f } },
    }
  };

  Camera camera_{glm::vec3(0.0f, 0.0f, 5.0f)};

  glm::mat4 projection_;
  glm::vec3 orig_bgcolor_;
  glm::vec3 bg_color_{0.0f, 0.0f, 0.0f};
  glm::vec2 last_mouse_;

  std::vector<Light> lights_;
  std::vector<glm::vec3> ssao_kernel_;

  bool wireframe_ = false;
  bool capture_mouse_ = false;
  bool capture_hold_ = false;
  bool reset_mouse_ = true;
  bool hide_interface_ = true;
  bool enable_debug_ = false;

  float aspect_ratio_ = 800.0f / 600.0f;

  unsigned int quad_vao_;
  unsigned int quad_vbo_;
  unsigned int g_buffer_;
  unsigned int g_position_;
  unsigned int g_normal_;
  unsigned int g_albedo_spec_;
  unsigned int depth_rbo_;
  unsigned int noise_texture_;
  unsigned int ssao_fbo_;
  unsigned int ssao_color_buffer_;

  int debug_mode_ = 1;
};
