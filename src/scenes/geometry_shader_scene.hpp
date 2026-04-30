
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


class GeometryShaderScene final : public Scene {
public:
  void Init(IAppContext* ctx) override {
    ctx_ = ctx;

    orig_bgcolor_ = ctx_->GetBackgroundColor();
    ctx_->SetBackgroundColor(environment_.bg_color);

    const auto window_size = ctx_->GetWindowSize();

    shader_ = Shader::FromFiles("resources/shaders/geometry_shader_scene/main.gs", "resources/shaders/geometry_shader_scene/main.vs", "resources/shaders/geometry_shader_scene/main.fs");
    explode_shader_ = Shader::FromFiles("resources/shaders/geometry_shader_scene/explode.gs", "resources/shaders/geometry_shader_scene/explode.vs", "resources/shaders/geometry_shader_scene/explode.fs");
    normal_shader_ = Shader::FromFiles("resources/shaders/geometry_shader_scene/normal.gs", "resources/shaders/geometry_shader_scene/normal.vs", "resources/shaders/geometry_shader_scene/normal.fs");

    backpack_model_ = Model::Load("resources/models/backpack/backpack.obj");

    projection_ = glm::perspective(glm::radians(camera_.fov), aspect_ratio_, 0.1f, 100.0f);

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

    if (animate_explode_) {
      explode_time_ += ctx_->GetFrameTime();
      while (explode_time_ > (2 * M_PI)) {
        explode_time_ -= (2 * M_PI);
      }
    }
  }

  void Render() override {
    glPolygonMode(GL_FRONT_AND_BACK, wireframe_ ? GL_LINE : GL_FILL);

    if (explode_ || render_normals_) {
      glm::mat4 view = camera_.GetViewMatrix();

      explode_shader_.Use();
      explode_shader_.SetMat4("view", view);
      explode_shader_.SetMat4("projection", projection_);

      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
      model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
      explode_shader_.SetMat4("model", model);

      float time = sinf(explode_time_);

      explode_shader_.SetFloat("time", time);
      backpack_model_.Draw(explode_shader_);

      if (render_normals_) {
        normal_shader_.Use();
        normal_shader_.SetMat4("view", view);
        normal_shader_.SetMat4("projection", projection_);
        normal_shader_.SetMat4("model", model);
        normal_shader_.SetFloat("time", time);
        backpack_model_.Draw(normal_shader_);
      }
    } else {
      shader_.Use();
      mesh_.Draw(shader_);
    }
  }

  void RenderInterface(int window_width, int window_height) override {
    constexpr auto padding = 5.0f;
    constexpr auto menu_bar_height = 32.0f;

    ImGui::PushID("GeometryShader");
    ImGui::SetNextWindowPos(ImVec2(window_width - padding, menu_bar_height - padding), ImGuiCond_FirstUseEver, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Scene Options")) {
      ImGui::Checkbox("Wireframe", &wireframe_);
      ImGui::NewLine();

      if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Explode", &explode_);
        ImGui::Checkbox("Animate explosion", &animate_explode_);
        ImGui::Checkbox("Render normals", &render_normals_);

        static float sin_time;
        sin_time = sin(explode_time_);
        if (ImGui::DragFloat("Explode time", &sin_time, 0.01f, -1.0f, 1.0f)) {
          explode_time_ = asinf(sin_time);
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
  }

  std::string Name() const override {
    return "Geometry Shader";
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

  Shader shader_;
  Shader explode_shader_;
  Shader normal_shader_;

  Camera camera_{glm::vec3(0.0f, 0.0f, 3.0f)};
  Environment environment_{
    .name = "Default",
    .bg_color = glm::vec3(0.0f, 0.0f, 0.0f),
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

  Mesh mesh_{
    Mesh::Type::Points,
    {
      { { -0.5f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } }, // top-left
      { {  0.5f,  0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } }, // top-right
      { {  0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } }, // bottom-right
      { { -0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } }, // bottom-left
    },
  };

  Model backpack_model_;

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
  bool explode_ = false;
  bool render_normals_ = false;
  bool animate_explode_ = true;

  float aspect_ratio_ = 800.0f / 600.0f;
  float explode_time_ = 0.0f;
};
