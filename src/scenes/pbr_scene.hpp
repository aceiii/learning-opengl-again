
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

class PBRScene final : public Scene {
public:
  static constexpr unsigned int kShadowWidth = 1024;
  static constexpr unsigned int kShadowHeight = 1024;

  void Init(IAppContext* ctx) override {
    ctx_ = ctx;

    orig_bgcolor_ = ctx_->GetBackgroundColor();
    ctx_->SetBackgroundColor(bg_color_);

    shader_ = Shader::FromFiles("resources/shaders/pbr_scene/main.vs", "resources/shaders/pbr_scene/main.fs");
    textured_shader_ = Shader::FromFiles("resources/shaders/pbr_scene/textured.vs", "resources/shaders/pbr_scene/textured.fs");
    env_shader_ = Shader::FromFiles("resources/shaders/pbr_scene/env.vs", "resources/shaders/pbr_scene/env.fs");
    bg_shader_ = Shader::FromFiles("resources/shaders/pbr_scene/bg.vs", "resources/shaders/pbr_scene/bg.fs");
    irradiance_shader_ = Shader::FromFiles("resources/shaders/pbr_scene/irradiance.vs", "resources/shaders/pbr_scene/irradiance.fs");
    prefilter_shader_ = Shader::FromFiles("resources/shaders/pbr_scene/prefilter.vs", "resources/shaders/pbr_scene/prefilter.fs");

    projection_ = glm::perspective(glm::radians(camera_.fov), aspect_ratio_, 0.1f, 100.0f);
    camera_.position = glm::vec3(19.0f, 3.0f, 18.0f);
    camera_.yaw = -130.0f;
    camera_.pitch = -7.5f;
    camera_.UpdateCameraVectors();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    InitSphere();

    LoadTextureGroup();
    BindTextureGroup();

    InitCubeMap();

    auto [width, height] = ctx_->GetFramebufferSize();
    glViewport(0, 0, width, height);
  }

  void InitCubeMap() {
    texture_hdr_ = Texture::Load("hdr", "resources/textures/hdr/relax_inn_seaview_suite_4k.hdr", {
      .hdr = true,
    });

    glGenFramebuffers(1, &capture_fbo_);
    glGenRenderbuffers(1, &capture_rbo_);

    glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo_);
    glBindRenderbuffer(GL_RENDERBUFFER, capture_rbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,  capture_rbo_);

    glGenTextures(1, &env_cube_map_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, env_cube_map_);
    for (unsigned int idx = 0; idx < 6; idx++) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + idx, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glm::mat4 capture_projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    std::array capture_views = {
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    };

    env_shader_.Use();
    env_shader_.SetInt("texture_equirectangular", 0);
    env_shader_.SetMat4("projection", capture_projection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_hdr_.id);

    glViewport(0, 0, 512, 512);
    glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo_);
    for (unsigned int idx = 0; idx < 6; idx++) {
      env_shader_.SetMat4("view", capture_views[idx]);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + idx, env_cube_map_, 0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      cube_mesh_.Draw(env_shader_);
    }

    glGenTextures(1, &prefilter_map_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilter_map_);
    for (unsigned int idx = 0; idx < 6; idx++) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + idx, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    prefilter_shader_.Use();
    prefilter_shader_.SetInt("texture_environment", 0);
    prefilter_shader_.SetMat4("projection", capture_projection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, env_cube_map_);
    glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo_);
    unsigned int max_mip_levels = 5;
    for (unsigned int mip = 0; mip < max_mip_levels; mip++) {
      unsigned int mip_width = static_cast<unsigned int>(128 * std::pow(0.5, mip));
      unsigned int mip_height = static_cast<unsigned int>(128 * std::pow(0.5, mip));
      glBindRenderbuffer(GL_RENDERBUFFER, capture_rbo_);
      glRenderbufferStorage(GL_RENDERBUFFER,  GL_DEPTH_COMPONENT24, mip_width, mip_height);
      glViewport(0, 0, mip_width, mip_height);

      float roughness = static_cast<float>(mip) / static_cast<float>(max_mip_levels - 1);
      prefilter_shader_.SetFloat("roughness", roughness);
      for (unsigned int idx = 0; idx < 6; idx++) {
        prefilter_shader_.SetMat4("view", capture_views[idx]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + idx, prefilter_map_, mip);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        cube_mesh_.Draw(prefilter_shader_);
      }
    }

    glGenTextures(1, &brdf_lut_map_);
    glBindTexture(GL_TEXTURE_2D, brdf_lut_map_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenTextures(1, &irradiance_map_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_map_);
    for (unsigned int idx = 0; idx < 6; idx++) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + idx, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo_);
    glBindRenderbuffer(GL_RENDERBUFFER, capture_rbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

    irradiance_shader_.Use();
    irradiance_shader_.SetInt("texture_environment", 0);
    irradiance_shader_.SetMat4("projection", capture_projection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, env_cube_map_);

    glViewport(0, 0, 32, 32);
    glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo_);
    for (unsigned int idx = 0; idx < 6; idx++) {
      irradiance_shader_.SetMat4("view", capture_views[idx]);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + idx, irradiance_map_, 0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      cube_mesh_.Draw(irradiance_shader_);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  void InitSphere() {
    glGenVertexArrays(1, &sphere_vao_);

    unsigned int vbo, ebo;
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uv;
    std::vector<glm::vec3> normals;
    std::vector<unsigned int> indices;

    const unsigned int x_segments = 64;
    const unsigned int y_segments = 64;

    for (int x = 0; x <= x_segments; x++) {
      for (int y = 0; y <= y_segments; y++) {
        float x_segment = static_cast<float>(x) / static_cast<float>(x_segments);
        float y_segment = static_cast<float>(y) / static_cast<float>(y_segments);
        float x_pos = std::cos(x_segment * 2.0f * M_PI) * std::sin(y_segment * M_PI);
        float y_pos = std::cos(y_segment * M_PI);
        float z_pos = std::sin(x_segment * 2.0 * M_PI) * std::sin(y_segment * M_PI);

        positions.push_back(glm::vec3(x_pos, y_pos, z_pos));
        uv.push_back(glm::vec2(x_segment, y_segment));
        normals.push_back(glm::vec3(x_pos, y_pos, z_pos));
      }
    }

    bool odd_row = false;
    for (int y = 0; y < y_segments; y++) {
      if (!odd_row) {
        for (int x = 0; x <= x_segments; x++) {
          indices.push_back(y * (x_segments + 1) + x);
          indices.push_back((y + 1) * (x_segments + 1) + x);
        }
      } else {
        for (int x = x_segments; x >= 0; --x) {
          indices.push_back((y + 1) * (x_segments + 1) + x);
          indices.push_back(y * (x_segments + 1) + x);
        }
      }
      odd_row = !odd_row;
    }

    sphere_index_count_ = static_cast<unsigned int>(indices.size());

    std::vector<float> data;
    for (auto i = 0; i < positions.size(); i++) {
      data.push_back(positions[i].x);
      data.push_back(positions[i].y);
      data.push_back(positions[i].z);

      if (normals.size() > 0) {
        data.push_back(normals[i].x);
        data.push_back(normals[i].y);
        data.push_back(normals[i].z);
      }

      if (uv.size() > 0) {
        data.push_back(uv[i].x);
        data.push_back(uv[i].y);
      }
    }

    glBindVertexArray(sphere_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    unsigned int stride = (3 + 2 + 3) * sizeof(float);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

    glBindVertexArray(0);
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

  void RenderSphere() {
    glBindVertexArray(sphere_vao_);
    glDrawElements(GL_TRIANGLE_STRIP, sphere_index_count_, GL_UNSIGNED_INT, 0);
  }

  void Render() override {
    glm::mat4 view = camera_.GetViewMatrix();

    auto& shader = show_textured_ ? textured_shader_ : shader_;

    shader.Use();
    shader.SetInt("texture_albedo", 0);
    shader.SetInt("texture_normal", 1);
    shader.SetInt("texture_metallic", 2);
    shader.SetInt("texture_roughness", 3);
    shader.SetInt("texture_ao", 4);
    shader.SetMat4("view", view);
    shader.SetMat4("projection", projection_);
    shader.SetVec3("viewPos", camera_.position);

    shader.SetVec3("albedo", albedo_);
    shader.SetFloat("ao", 1.0f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_map_);

    shader.SetVec3("ambient", ambient_);
    for (auto idx = 0; idx < lights_.size(); idx++) {
      shader.SetVec3("lights[" + std::to_string(idx) + "].Position", lights_[idx].position);
      shader.SetVec3("lights[" + std::to_string(idx) + "].Color", lights_[idx].color);
    }

    glm::mat4 model = glm::mat4(1.0f);

    const int num_rows = 7;
    const int num_columns = 7;
    const float spacing = 2.5f;

    for (int row = 0; row < num_rows; row++) {
      shader.SetFloat("metallic", static_cast<float>(row) / static_cast<float>(num_rows));
      for (int col = 0; col < num_columns; col++) {
        shader.SetFloat("roughness", glm::clamp(static_cast<float>(col) / static_cast<float>(num_columns), 0.05f, 1.0f));

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3((col - (num_columns / 2)) * spacing, (row - (num_rows / 2)) * spacing, 0.0f));
        model = glm::scale(model, glm::vec3(1.0f));
        shader.SetMat4("model", model);
        shader.SetMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        RenderSphere();
      }
    }

    bg_shader_.Use();
    bg_shader_.SetMat4("view", view);
    bg_shader_.SetMat4("projection", projection_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, show_irradiance_ ? irradiance_map_ : env_cube_map_);
    cube_mesh_.Draw(bg_shader_);
  }

  void RenderInterface(int window_width, int window_height) override {
    constexpr auto padding = 5.0f;
    constexpr auto menu_bar_height = 32.0f;

    ImGui::PushID("PBR");
    ImGui::SetNextWindowPos(ImVec2(window_width - padding, menu_bar_height - padding), ImGuiCond_FirstUseEver, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Scene Options")) {
      ImGui::Checkbox("Wireframe", &wireframe_);
      ImGui::Checkbox("Use textures", &show_textured_);
      if (ImGui::BeginCombo("Texture", kTextureGroups[selected_texture_idx_].name.c_str())) {
        for (auto idx = 0; idx < kTextureGroups.size(); idx++) {
          auto& texture_group = kTextureGroups[idx];
          if (ImGui::Selectable(texture_group.name.c_str(), idx == selected_texture_idx_) && idx != selected_texture_idx_) {
            UnloadTextureGroup();
            selected_texture_idx_ = idx;
            LoadTextureGroup();
            BindTextureGroup();
          }
        }
        ImGui::EndCombo();
      }
      ImGui::NewLine();
      if (ImGui::CollapsingHeader("Environment", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Show irradiance map", &show_irradiance_);
      }

      if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::ColorEdit3("Albedo", glm::value_ptr(albedo_));
      }

      if (ImGui::CollapsingHeader("Lights")) {
        ImGui::ColorEdit3("Ambient", glm::value_ptr(ambient_), ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
        ImGui::NewLine();
        for (auto idx = 0; idx < lights_.size(); idx++) {
          auto& light = lights_[idx];
          ImGui::PushID(("Light##" + std::to_string(idx)).c_str());
          ImGui::Text("Light %d", idx+1);
          ImGui::DragFloat3("Position", glm::value_ptr(light.position));
          ImGui::ColorEdit3("Color", glm::value_ptr(light.color), ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
          ImGui::PopID();
        }
      }

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
    textured_shader_.Destroy();
  }

  std::string Name() const override {
    return "PBR";
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

  void BindTextureGroup() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_albedo_.id);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture_normal_.id);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texture_metallic_.id);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, texture_roughness_.id);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, texture_ao_.id);
  }

  void LoadTextureGroup() {
    auto& texture_group = kTextureGroups[selected_texture_idx_];
    texture_albedo_ = Texture::Load("albedo", texture_group.albedo);
    texture_normal_ = Texture::Load("normal", texture_group.normal);
    texture_metallic_ = Texture::Load("metallic", texture_group.metallic);
    texture_roughness_ = Texture::Load("roughness", texture_group.roughness);
    texture_ao_ = Texture::Load("ao", texture_group.ao);
  }

  void UnloadTextureGroup() {
    texture_albedo_.Unload();
    texture_normal_.Unload();
    texture_metallic_.Unload();
    texture_roughness_.Unload();
    texture_ao_.Unload();
  }

  struct Light {
    glm::vec3 position;
    glm::vec3 color;
    float radius;
  };

  struct TextureGroup {
    std::string name;
    std::string path;
    std::string albedo;
    std::string normal;
    std::string metallic;
    std::string roughness;
    std::string ao;
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
    "Ambient Occlusion"s,
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

  inline static const std::array kTextureGroups{
    TextureGroup{
      .name = "rustediron",
      .albedo = "resources/textures/rustediron-streaks2-bl/rustediron-streaks_basecolor.png",
      .normal = "resources/textures/rustediron-streaks2-bl/rustediron-streaks_normal.png",
      .metallic = "resources/textures/rustediron-streaks2-bl/rustediron-streaks_metallic.png",
      .roughness = "resources/textures/rustediron-streaks2-bl/rustediron-streaks_roughness.png",
      .ao = "resources/textures/1x1-white.png",
    },
    TextureGroup{
      .name = "rustediron-alt",
      .albedo = "resources/textures/rustediron1-alt2-bl/rustediron2_basecolor.png",
      .normal = "resources/textures/rustediron1-alt2-bl/rustediron2_normal.png",
      .metallic = "resources/textures/rustediron1-alt2-bl/rustediron2_metallic.png",
      .roughness = "resources/textures/rustediron1-alt2-bl/rustediron2_roughness.png",
      .ao = "resources/textures/1x1-white.png",
    },
    TextureGroup{
      .name = "scuffed-metal",
      .albedo = "resources/textures/scuffed-metal1-bl/scuffed-metal1_albedo.png",
      .normal = "resources/textures/scuffed-metal1-bl/scuffed-metal1_normal-ogl.png",
      .metallic = "resources/textures/scuffed-metal1-bl/scuffed-metal1_metallic.png",
      .roughness = "resources/textures/scuffed-metal1-bl/scuffed-metal1_roughness.png",
      .ao = "resources/textures/scuffed-metal1-bl/scuffed-metal1_ao.png",
    },
  };

  Shader shader_;
  Shader textured_shader_;
  Shader env_shader_;
  Shader bg_shader_;
  Shader irradiance_shader_;
  Shader prefilter_shader_;

  Texture texture_albedo_;
  Texture texture_normal_;
  Texture texture_metallic_;
  Texture texture_roughness_;
  Texture texture_ao_;
  Texture texture_hdr_;

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
    },
    {},
    // {
    //   Texture::Load("diffuse", "resources/textures/wood.png", { .linear = true }),
    //   Texture::Load("specular", "resources/textures/wood.png", { .linear = false }),
    // }
  };

  Camera camera_{glm::vec3(0.0f, 0.0f, 5.0f)};

  std::vector<Light> lights_ = {{
    Light{
      .position = glm::vec3(-10.0f, 10.0f, 10.0f),
      .color = glm::vec3(300.0f, 300.0f, 300.0f),
    },
    Light{
      .position = glm::vec3(10.0f, 10.0f, 10.0f),
      .color = glm::vec3(300.0f, 300.0f, 300.0f),
    },
    Light{
      .position = glm::vec3(-10.0f, -10.0f, 10.0f),
      .color = glm::vec3(300.0f, 300.0f, 300.0f),
    },
    Light{
      .position = glm::vec3(10.0f, -10.0f, 10.0f),
      .color = glm::vec3(300.0f, 300.0f, 300.0f),
    },
  }};

  glm::mat4 projection_;
  glm::vec3 orig_bgcolor_;
  glm::vec3 bg_color_{0.0f, 0.0f, 0.0f};
  glm::vec2 last_mouse_;

  glm::vec3 albedo_ = glm::vec3(0.5f, 0.0f, 0.0f);
  glm::vec3 ambient_ = glm::vec3(0.3f);

  bool wireframe_ = false;
  bool capture_mouse_ = false;
  bool capture_hold_ = false;
  bool reset_mouse_ = true;
  bool hide_interface_ = true;
  bool show_textured_ = false;
  bool show_irradiance_ = false;

  float aspect_ratio_ = 800.0f / 600.0f;

  unsigned int sphere_vao_;
  unsigned int selected_texture_idx_ = 0;
  unsigned int capture_fbo_;
  unsigned int capture_rbo_;
  unsigned int env_cube_map_;
  unsigned int irradiance_map_;
  unsigned int prefilter_map_;
  unsigned int brdf_lut_map_;

  int sphere_index_count_ = 0;
};
