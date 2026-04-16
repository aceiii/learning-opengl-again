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


class UboScene final : public Scene {
public:
  void Init(IAppContext* ctx) override {
    ctx_ = ctx;

    orig_bgcolor_ = ctx_->GetBackgroundColor();
    ctx_->SetBackgroundColor(environment_.bg_color);

    const auto window_size = ctx_->GetWindowSize();

    red_shader_ = Shader::FromFiles("resources/shaders/ubo_scene/main.vs", "resources/shaders/ubo_scene/red.fs");
    green_shader_ = Shader::FromFiles("resources/shaders/ubo_scene/main.vs", "resources/shaders/ubo_scene/green.fs");
    yellow_shader_ = Shader::FromFiles("resources/shaders/ubo_scene/main.vs", "resources/shaders/ubo_scene/yellow.fs");
    blue_shader_ = Shader::FromFiles("resources/shaders/ubo_scene/main.vs", "resources/shaders/ubo_scene/blue.fs");

    glGenBuffers(1, &ubo_);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    red_shader_.SetBlockBinding("Matrices", 0);
    green_shader_.SetBlockBinding("Matrices", 0);
    yellow_shader_.SetBlockBinding("Matrices", 0);
    blue_shader_.SetBlockBinding("Matrices", 0);

    glBindBufferRange(GL_UNIFORM_BUFFER, 0, ubo_, 0, 2 * sizeof(glm::mat4));

    projection_ = glm::perspective(glm::radians(camera_.fov), aspect_ratio_, 0.1f, 100.0f);

    glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projection_));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

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
    glm::mat4 skybox_view =  glm::mat4(glm::mat3(view));
    glm::mat4 model = glm::mat4(1.0f);

    glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(view));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    model = glm::translate(glm::mat4(1.0f), glm::vec3(-0.75f, 0.75f, 0.0f));
    red_shader_.Use();
    red_shader_.SetMat4("model", model);
    cube_mesh_.Draw(red_shader_);

    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.75f, 0.75f, 0.0f));
    green_shader_.Use();
    green_shader_.SetMat4("model", model);
    cube_mesh_.Draw(green_shader_);

    model = glm::translate(glm::mat4(1.0f), glm::vec3(-0.75f, -0.75f, 0.0f));
    yellow_shader_.Use();
    yellow_shader_.SetMat4("model", model);
    cube_mesh_.Draw(yellow_shader_);

    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.75f, -0.75f, 0.0f));
    blue_shader_.Use();
    blue_shader_.SetMat4("model", model);
    cube_mesh_.Draw(blue_shader_);
  }

  void RenderInterface(int window_width, int window_height) override {
    constexpr auto padding = 5.0f;
    constexpr auto menu_bar_height = 32.0f;

    ImGui::PushID("UniformBufferObject");
    ImGui::SetNextWindowPos(ImVec2(window_width - padding, menu_bar_height - padding), ImGuiCond_FirstUseEver, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Scene Options")) {
      ImGui::Checkbox("Wireframe", &wireframe_);
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

    red_shader_.Destroy();
    green_shader_.Destroy();
    yellow_shader_.Destroy();
    blue_shader_.Destroy();
  }

  std::string Name() const override {
    return "Uniform Buffer Object";
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

  IAppContext* ctx_ = nullptr;

  inline static const float kDefaultYaw = -90.0f;
  inline static const float kDefaultPitch = 0.0f;
  inline static const float kMinPitch = -89.0f;
  inline static const float kMaxPitch = 89.0f;

  inline static const std::array kVegetationPositions{
    glm::vec3(-1.5f,  0.0f, -0.48f),
    glm::vec3( 1.5f,  0.0f,  0.51f),
    glm::vec3( 0.0f,  0.0f,  0.7f),
    glm::vec3(-0.3f,  0.0f, -2.3f),
    glm::vec3( 0.5f,  0.0f, -0.6f),
  };

  inline static const std::array kBlendFuncModes{
    NamedEnum{ "0", GL_ZERO },
    NamedEnum{ "1", GL_ONE },
    NamedEnum{ "src color", GL_SRC_COLOR },
    NamedEnum{ "1 - src color", GL_ONE_MINUS_SRC_COLOR },
    NamedEnum{ "1 - dst color", GL_ONE_MINUS_DST_COLOR },
    NamedEnum{ "src alpha", GL_SRC_ALPHA },
    NamedEnum{ "1 - src alpha", GL_ONE_MINUS_SRC_ALPHA },
    NamedEnum{ "dst alpha", GL_DST_ALPHA },
    NamedEnum{ "1 - dst alpha", GL_ONE_MINUS_DST_ALPHA },
    NamedEnum{ "constant color", GL_CONSTANT_COLOR },
    NamedEnum{ "constant alpha", GL_CONSTANT_ALPHA },
    NamedEnum{ "1 - constant alpha", GL_ONE_MINUS_CONSTANT_ALPHA },
  };

  inline static const std::array kBlendEquations{
    NamedEnum{ "Add", GL_FUNC_ADD },
    NamedEnum( "Subtract", GL_FUNC_SUBTRACT ),
    NamedEnum( "Reverse Subtract", GL_FUNC_REVERSE_SUBTRACT ),
    NamedEnum( "Min", GL_MIN ),
    NamedEnum( "Max", GL_MAX ),
  };

  inline static const std::array kCullFaceModes{
    NamedEnum{ "Back", GL_BACK },
    NamedEnum{ "Front", GL_FRONT },
    NamedEnum{ "Front and Back", GL_FRONT_AND_BACK },
  };

  inline static const std::array kFrontFaceModes{
    NamedEnum{ "Clock-wise", GL_CW },
    NamedEnum{ "Counter Clock-wise", GL_CCW },
  };

  inline static const std::array kShaderModes {
    std::string{"Regular"},
    std::string{"Invert"},
    std::string{"Greyscale"},
    std::string{"Greyscale 2"},
    std::string{"Kernel"},
  };

  inline static const std::array kKernelModes {
    KernelMode{ "Identity",         {  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f } },
    KernelMode{ "Sharpen",          { -1.0f, -1.0f, -1.0f, -1.0f,  9.0f, -1.0f, -1.0f, -1.0f, -1.0f } },
    KernelMode{ "Box Blur",         {  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f } },
    KernelMode{ "Gaussian Blur",    {  1.0f,  2.0f,  1.0f,  2.0f,  4.0f,  2.0f,  1.0f,  2.0f,  1.0f } },
    KernelMode{ "Edge Detection",   {  1.0f,  1.0f,  1.0f,  1.0f, -8.0f,  1.0f,  1.0f,  1.0f,  1.0f } },
    KernelMode{ "Edge Detection 2", { -1.0f, -1.0f, -1.0f, -1.0f,  8.0f, -1.0f, -1.0f, -1.0f, -1.0f } },
  };

  Shader red_shader_;
  Shader green_shader_;
  Shader yellow_shader_;
  Shader blue_shader_;

  Camera camera_{glm::vec3(0.0f, 0.0f, 3.0f)};
  Environment environment_{
    .name = "Default",
    .bg_color = glm::vec3(0.7294f, 0.7294f, 0.5078f),
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

  Mesh cube_mesh_{
    {
      // Back face
      { { -0.5f, -0.5f, -0.5f }, { 0.0f,  0.0f, -1.0f }, { 0.0f, 0.0f } }, // Bottom-left
      { {  0.5f,  0.5f, -0.5f }, { 0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f } }, // top-right
      { {  0.5f, -0.5f, -0.5f }, { 0.0f,  0.0f, -1.0f }, { 1.0f, 0.0f } }, // bottom-right
      { {  0.5f,  0.5f, -0.5f }, { 0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f } }, // top-right
      { { -0.5f, -0.5f, -0.5f }, { 0.0f,  0.0f, -1.0f }, { 0.0f, 0.0f } }, // bottom-left
      { { -0.5f,  0.5f, -0.5f }, { 0.0f,  0.0f, -1.0f }, { 0.0f, 1.0f } }, // top-left

      // Front face
      { { -0.5f, -0.5f,  0.5f }, { 0.0f,  0.0f, 1.0f }, { 0.0f, 0.0f } }, // bottom-left
      { {  0.5f, -0.5f,  0.5f }, { 0.0f,  0.0f, 1.0f }, { 1.0f, 0.0f } }, // bottom-right
      { {  0.5f,  0.5f,  0.5f }, { 0.0f,  0.0f, 1.0f }, { 1.0f, 1.0f } }, // top-right
      { {  0.5f,  0.5f,  0.5f }, { 0.0f,  0.0f, 1.0f }, { 1.0f, 1.0f } }, // top-right
      { { -0.5f,  0.5f,  0.5f }, { 0.0f,  0.0f, 1.0f }, { 0.0f, 1.0f } }, // top-left
      { { -0.5f, -0.5f,  0.5f }, { 0.0f,  0.0f, 1.0f }, { 0.0f, 0.0f } }, // bottom-left

      // Left face
      { { -0.5f,  0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f } }, // top-right
      { { -0.5f,  0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } }, // top-left
      { { -0.5f, -0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } }, // bottom-left
      { { -0.5f, -0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } }, // bottom-left
      { { -0.5f, -0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } }, // bottom-right
      { { -0.5f,  0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f } }, // top-right

      // Right face
      { { 0.5f,  0.5f,  0.5f }, { 1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f } }, // top-left
      { { 0.5f, -0.5f, -0.5f }, { 1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } }, // bottom-right
      { { 0.5f,  0.5f, -0.5f }, { 1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } }, // top-right
      { { 0.5f, -0.5f, -0.5f }, { 1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } }, // bottom-right
      { { 0.5f,  0.5f,  0.5f }, { 1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f } }, // top-left
      { { 0.5f, -0.5f,  0.5f }, { 1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } }, // bottom-left

      // Bottom face
      { { -0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f,  0.0f }, { 0.0f, 1.0f } }, // top-right
      { {  0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f,  0.0f }, { 1.0f, 1.0f } }, // top-left
      { {  0.5f, -0.5f,  0.5f }, { 0.0f, -1.0f,  0.0f }, { 1.0f, 0.0f } }, // bottom-left
      { {  0.5f, -0.5f,  0.5f }, { 0.0f, -1.0f,  0.0f }, { 1.0f, 0.0f } }, // bottom-left
      { { -0.5f, -0.5f,  0.5f }, { 0.0f, -1.0f,  0.0f }, { 0.0f, 0.0f } }, // bottom-right
      { { -0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f,  0.0f }, { 0.0f, 1.0f } }, // top-right

      // Top face
      { { -0.5f,  0.5f, -0.5f }, { 0.0f,  1.0f,  0.0f }, { 0.0f, 1.0f } }, // top-left
      { {  0.5f,  0.5f,  0.5f }, { 0.0f,  1.0f,  0.0f }, { 1.0f, 0.0f } }, // bottom-right
      { {  0.5f,  0.5f, -0.5f }, { 0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f } }, // top-right
      { {  0.5f,  0.5f,  0.5f }, { 0.0f,  1.0f,  0.0f }, { 1.0f, 0.0f } }, // bottom-right
      { { -0.5f,  0.5f, -0.5f }, { 0.0f,  1.0f,  0.0f }, { 0.0f, 1.0f } }, // top-left
      { { -0.5f,  0.5f,  0.5f }, { 0.0f,  1.0f,  0.0f }, { 0.0f, 0.0f } },  // bottom-left
    },
    {},
    {
      Texture::Load("diffuse", "resources/textures/container.jpg"),
    },
  };

  Mesh sky_mesh_{
    {

      { { -1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { { -1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { {  1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { {  1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { {  1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { { -1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },

      { { -1.0f, -1.0f,  1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { { -1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { { -1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { { -1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { { -1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { { -1.0f, -1.0f,  1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },

      { {  1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { {  1.0f, -1.0f,  1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { {  1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { {  1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { {  1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { {  1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },

      { { -1.0f, -1.0f,  1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { { -1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { {  1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { {  1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { {  1.0f, -1.0f,  1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { { -1.0f, -1.0f,  1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },

      { { -1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { {  1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { {  1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { {  1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { { -1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { { -1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },

      { { -1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { { -1.0f, -1.0f,  1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { {  1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { {  1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { { -1.0f, -1.0f,  1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
      { {  1.0f, -1.0f,  1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
    },
    {},
    {},
  };


  int selected_environment_ = 0;
  int selected_depth_func_ = 2;
  int selected_cull_face_ = 1;
  int selected_front_face_ = 0;
  int shader_mode_ = 0;
  int kernel_mode_ = 0;

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

  unsigned int ubo_;
};
