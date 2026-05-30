

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


class BloomScene final : public Scene {
public:
  static constexpr unsigned int kShadowWidth = 1024;
  static constexpr unsigned int kShadowHeight = 1024;

  void Init(IAppContext* ctx) override {
    ctx_ = ctx;

    orig_bgcolor_ = ctx_->GetBackgroundColor();
    ctx_->SetBackgroundColor(bg_color_);

    shader_ = Shader::FromFiles("resources/shaders/bloom_scene/main.vs", "resources/shaders/bloom_scene/main.fs");
    light_shader_ = Shader::FromFiles("resources/shaders/bloom_scene/light.vs", "resources/shaders/bloom_scene/light.fs");
    blur_shader_ = Shader::FromFiles("resources/shaders/bloom_scene/blur.vs", "resources/shaders/bloom_scene/blur.fs");
    bloom_shader_ = Shader::FromFiles("resources/shaders/bloom_scene/bloom.vs", "resources/shaders/bloom_scene/bloom.fs");

    projection_ = glm::perspective(glm::radians(camera_.fov), aspect_ratio_, 0.1f, 100.0f);
    camera_.position = glm::vec3(9.5f, 1.0f, 6.0f);
    camera_.yaw = -140.0f;
    camera_.pitch = 10.0f;
    camera_.UpdateCameraVectors();

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

    auto [window_width, window_height] = ctx_->GetFramebufferSize();

    glGenFramebuffers(1, &hdr_fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, hdr_fbo_);

    glGenTextures(2, color_buffers_.data());
    for (auto idx = 0; idx < color_buffers_.size(); idx++) {
      glBindTexture(GL_TEXTURE_2D, color_buffers_[idx]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window_width, window_height, 0, GL_RGBA, GL_FLOAT, nullptr);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + idx, GL_TEXTURE_2D, color_buffers_[idx], 0);
    }

    glGenRenderbuffers(1, &depth_rbo_);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, window_width, window_height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rbo_);

    std::array<GLenum, 2> attachments{ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments.data());

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      LogWarning("Framebuffer not complete: hdr_fbo_");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenFramebuffers(2, pingpong_fbo_.data());
    glGenTextures(2, pingpong_color_buffers_.data());
    for (auto idx = 0; idx < pingpong_fbo_.size(); idx++) {
      glBindFramebuffer(GL_FRAMEBUFFER, pingpong_fbo_[idx]);
      glBindTexture(GL_TEXTURE_2D, pingpong_color_buffers_[idx]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window_width, window_height, 0, GL_RGBA, GL_FLOAT, nullptr);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpong_color_buffers_[idx], 0);
      glDrawBuffer(GL_COLOR_ATTACHMENT0);

      if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LogWarning("Framebuffer not complete: pingpong_fbo_[%d]", idx);
      }
    }
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

    glm::mat4 model = glm::mat4(1.0f);

    shader_.Use();
    shader_.SetMat4("view", camera_.GetViewMatrix());
    shader_.SetMat4("projection", projection_);
    shader_.SetVec3("viewPos", camera_.position);

    for (unsigned int i = 0; i < kLightPositions.size(); i++) {
      shader_.SetVec3("lights[" + std::to_string(i) + "].Position", kLightPositions[i]);
      shader_.SetVec3("lights[" + std::to_string(i) + "].Color", kLightColors[i]);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_wood_.id);

    model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(12.5f, 0.5f, 12.5f));
    shader_.SetMat4("model", model);
    cube_mesh_.Draw(shader_);

    // glBindTexture(GL_TEXTURE_2D, texture_container_.id);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 1.5f, 0.0f));
    model = glm::scale(model, glm::vec3(0.5f));
    shader_.SetMat4("model", model);
    cube_mesh_.Draw(shader_);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(2.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(0.5f));
    shader_.SetMat4("model", model);
    cube_mesh_.Draw(shader_);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-1.0f, -1.0f, 2.0f));
    model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f)));
    shader_.SetMat4("model", model);
    cube_mesh_.Draw(shader_);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 2.7f, 4.0f));
    model = glm::rotate(model, glm::radians(23.0f), glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f)));
    model = glm::scale(model, glm::vec3(1.25f));
    shader_.SetMat4("model", model);
    cube_mesh_.Draw(shader_);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-2.0f, 1.0f, -3.0f));
    model = glm::rotate(model, glm::radians(124.0f), glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f)));
    shader_.SetMat4("model", model);
    cube_mesh_.Draw(shader_);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-3.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(0.5f));
    shader_.SetMat4("model", model);
    cube_mesh_.Draw(shader_);

    light_shader_.Use();
    light_shader_.SetMat4("view", camera_.GetViewMatrix());
    light_shader_.SetMat4("projection", projection_);
    light_shader_.SetVec3("viewPos", camera_.position);
    for (auto idx = 0; idx < kLightPositions.size(); idx++) {
      model = glm::mat4(1.0f);
      model = glm::translate(model, kLightPositions[idx]);
      model = glm::scale(model, glm::vec3(0.25f));
      light_shader_.SetMat4("model", model);
      light_shader_.SetVec3("lightColor", kLightColors[idx]);
      cube_mesh_.Draw(light_shader_);
    }
  }

  void Render() override {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, hdr_fbo_);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    RenderScene();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    bool horizontal = true;
    bool first_iteration = true;
    const unsigned int amount = 10;
    blur_shader_.Use();
    for (auto idx = 0; idx < amount; idx++) {
      glBindFramebuffer(GL_FRAMEBUFFER, pingpong_fbo_[horizontal]);
      glClear(GL_COLOR_BUFFER_BIT);
      blur_shader_.SetBool("horizontal", horizontal);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, first_iteration ? color_buffers_[1] : pingpong_color_buffers_[!horizontal]);
      blur_shader_.SetInt("image", 0);
      RenderQuad();

      horizontal = !horizontal;
      first_iteration = false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, color_buffers_[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pingpong_color_buffers_[!horizontal]);
    bloom_shader_.Use();
    bloom_shader_.SetInt("texture_scene", 0);
    bloom_shader_.SetInt("texture_blur", 1);
    bloom_shader_.SetFloat("exposure", exposure_);
    bloom_shader_.SetBool("enableBloom", enable_bloom_);
    bloom_shader_.SetBool("enableScene", enable_scene_);
    RenderQuad();
  }

  void RenderInterface(int window_width, int window_height) override {
    constexpr auto padding = 5.0f;
    constexpr auto menu_bar_height = 32.0f;

    ImGui::PushID("Bloom");
    ImGui::SetNextWindowPos(ImVec2(window_width - padding, menu_bar_height - padding), ImGuiCond_FirstUseEver, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Scene Options")) {
      ImGui::Checkbox("Wireframe", &wireframe_);
      ImGui::NewLine();
      ImGui::Checkbox("Enable Scene", &enable_scene_);
      ImGui::Checkbox("Enable Bloom", &enable_bloom_);
      ImGui::DragFloat("Exposure", &exposure_, 0.01f, 0.0f, 100.f);

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
    return "Bloom";
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

  struct NamedEnum {
    std::string name;
    GLenum func;
  };

  struct KernelMode {
    std::string name;
    std::array<float, 9> kernel;
  };

  struct TextureGroup {
    std::string name;
    Texture diffuse;
    Texture normal;
    Texture depth;
  };

  IAppContext* ctx_ = nullptr;

  inline static const float kDefaultYaw = -90.0f;
  inline static const float kDefaultPitch = 0.0f;
  inline static const float kMinPitch = -89.0f;
  inline static const float kMaxPitch = 89.0f;

  inline static const std::array kLightPositions{
    glm::vec3( 0.0f, 0.5f,  1.5f),
    glm::vec3(-4.0f, 0.5f, -3.0f),
    glm::vec3( 3.0f, 0.5f,  1.0f),
    glm::vec3(-0.8f, 2.4f,-1.0f),
  };

  inline static const std::array kLightColors{
    glm::vec3(5.0f, 5.0f, 5.0f),
    glm::vec3(10.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 0.0f, 15.0f),
    glm::vec3(0.0f, 5.0f, 0.0f),
  };

  Shader shader_;
  Shader light_shader_;
  Shader blur_shader_;
  Shader bloom_shader_;

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

  Texture texture_wood_ = Texture::Load("diffuse", "resources/textures/wood.png", { .linear = true });
  Texture texture_container_ = Texture::Load("diffuse", "resources/textures/container.jpg", { .linear = true });

  Camera camera_{glm::vec3(0.0f, 0.0f, 5.0f)};

  glm::mat4 projection_;
  glm::vec3 orig_bgcolor_;
  glm::vec3 bg_color_{0.0f, 0.0f, 0.0f};
  glm::vec2 last_mouse_;

  bool wireframe_ = false;
  bool capture_mouse_ = false;
  bool capture_hold_ = false;
  bool reset_mouse_ = true;
  bool hide_interface_ = true;
  bool enable_bloom_ = true;
  bool enable_scene_ = true;

  float aspect_ratio_ = 800.0f / 600.0f;
  float exposure_ = 1.0f;

  unsigned int quad_vao_;
  unsigned int quad_vbo_;
  unsigned int hdr_fbo_;
  unsigned int depth_rbo_;
  std::array<unsigned int, 2> color_buffers_;
  std::array<unsigned int, 2> pingpong_fbo_;
  std::array<unsigned int, 2> pingpong_color_buffers_;
};
