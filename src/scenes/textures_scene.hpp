#pragma once

#include <string>
#include <tuple>
#include <vector>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <stb_image.h>

#include "../image.hpp"
#include "../scene.hpp"
#include "../shader.hpp"


class TexturesScene final : public Scene {
public:
  void Init(IAppContext* ctx) override {
    textures_.push_back(LoadTexture(GL_TEXTURE0, "resources/textures/container.jpg"));
    textures_.push_back(LoadTexture(GL_TEXTURE1, "resources/textures/awesomeface.png"));

    shader_ = Shader::FromFiles("resources/shaders/textures_scene/main.vs", "resources/shaders/textures_scene/main.fs");

    vaos_.resize(2);
    vbos_.resize(2);

    glGenVertexArrays(vaos_.size(), vaos_.data());
    glGenBuffers(vbos_.size(), vbos_.data());

    glGenBuffers(1, &ebo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kIndices), kIndices.data(), GL_STATIC_DRAW);

    glBindVertexArray(vaos_[0]);
    glBindBuffer(GL_ARRAY_BUFFER, vbos_[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);

    glBindVertexArray(vaos_[1]);
    glBindBuffer(GL_ARRAY_BUFFER, vbos_[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kVerticesExtendTexCoords), kVerticesExtendTexCoords.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
  }

  void Update(float dt) override {
  }

  void Render() override {
    glPolygonMode(GL_FRONT_AND_BACK, wireframe_ ? GL_LINE : GL_FILL);

    shader_.Use();
    shader_.SetFloat("xOffset", horizontal_offset_);
    shader_.SetFloat("yOffset", vertical_offset_);
    shader_.SetFloat("outputMix", show_texture_ ? 1.0f : 0.0f);
    shader_.SetFloat("vertexBlend", blend_vertex_color_ ? 1.0f : 0.0f);
    shader_.SetFloat("textureBlend", texture_blend_);
    shader_.SetBool("mirrorFace", mirror_happy_face_);
    shader_.SetInt("texture1", 0);
    shader_.SetInt("texture2", 1);

    glBindVertexArray(alternate_tex_coords_ ? vaos_[1] : vaos_[0]);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  }

  void RenderInterface(int window_width, int window_height) override {
    constexpr auto padding = 5.0f;
    constexpr auto menu_bar_height = 32.0f;

    ImGui::PushID("ShadersScene");
    ImGui::SetNextWindowPos(ImVec2(window_width - padding, menu_bar_height - padding), ImGuiCond_FirstUseEver, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Scene Options")) {
      ImGui::Checkbox("Wireframe", &wireframe_);
      ImGui::Checkbox("Texture", &show_texture_);
      ImGui::DragFloat("Texture Blend", &texture_blend_, 0.01f, 0.0f, 1.0f);
      ImGui::Checkbox("Blend Vertex Color", &blend_vertex_color_);
      ImGui::Checkbox("Mirror Happy Face", &mirror_happy_face_);
      ImGui::Checkbox("Alternate Tex Coords", &alternate_tex_coords_);

      if (ImGui::BeginCombo("Texture Wrap S", std::get<0>(kTextureWrapModes[texture_wrap_s_]))) {
        for (auto idx = 0; idx < kTextureWrapModes.size(); idx++) {
          const auto& tuple = kTextureWrapModes[idx];
          if (ImGui::Selectable(std::get<0>(tuple), idx == texture_wrap_s_)) {
            texture_wrap_s_ = idx;
            glActiveTexture(GL_TEXTURE0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, std::get<1>(tuple));
            glActiveTexture(GL_TEXTURE1);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, std::get<1>(tuple));
          }
        }
        ImGui::EndCombo();
      }

      if (ImGui::BeginCombo("Texture Wrap T", std::get<0>(kTextureWrapModes[texture_wrap_t_]))) {
        for (auto idx = 0; idx < kTextureWrapModes.size(); idx++) {
          const auto& tuple = kTextureWrapModes[idx];
          if (ImGui::Selectable(std::get<0>(tuple), idx == texture_wrap_t_)) {
            texture_wrap_t_ = idx;
            glActiveTexture(GL_TEXTURE0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, std::get<1>(tuple));
            glActiveTexture(GL_TEXTURE1);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, std::get<1>(tuple));
          }
        }
        ImGui::EndCombo();
      }

      if (ImGui::BeginCombo("Texture Min Filter", std::get<0>(kTextureFilters[texture_min_filter_]))) {
        for (auto idx = 0; idx < kTextureFilters.size(); idx++) {
          const auto& tuple = kTextureFilters[idx];
          if (ImGui::Selectable(std::get<0>(tuple), idx == texture_min_filter_)) {
            texture_min_filter_ = idx;
            glActiveTexture(GL_TEXTURE0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, std::get<1>(tuple));
            glActiveTexture(GL_TEXTURE1);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, std::get<1>(tuple));
          }
        }
        ImGui::EndCombo();
      }

      if (ImGui::BeginCombo("Texture Mag Filter", std::get<0>(kTextureFilters[texture_mag_filter_]))) {
        for (auto idx = 0; idx < kTextureFilters.size(); idx++) {
          const auto& tuple = kTextureFilters[idx];
          if (ImGui::Selectable(std::get<0>(tuple), idx == texture_mag_filter_)) {
            texture_mag_filter_ = idx;
            glActiveTexture(GL_TEXTURE0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, std::get<1>(tuple));
            glActiveTexture(GL_TEXTURE1);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, std::get<1>(tuple));
          }
        }
        ImGui::EndCombo();
      }

      ImGui::DragFloat("X Offset", &horizontal_offset_, 0.01f, -2.0f, 2.0f);
      ImGui::DragFloat("Y Offset", &vertical_offset_, 0.01f, -2.0f, 2.0f);
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
    return "Textures";
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
     0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
     0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
    -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
    -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f,
  };

  inline static const std::array kVerticesExtendTexCoords{
     0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   2.0f, 2.0f,
     0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   2.0f, 0.0f,
    -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
    -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 2.0f,
  };

  inline static const std::array kIndices{
    0, 1, 3,
    1, 2, 3,
  };

  Shader shader_;
  std::vector<unsigned int> vaos_;
  std::vector<unsigned int> vbos_;
  std::vector<unsigned int> textures_;
  unsigned int ebo_ = 0;

  bool wireframe_ = false;
  bool show_texture_ = true;
  bool blend_vertex_color_ = false;
  bool mirror_happy_face_ = false;
  bool alternate_tex_coords_ = false;
  float texture_blend_ = 0.2f;
  float vertical_offset_ = 0.0f;
  float horizontal_offset_ = 0.0f;

  inline static const std::array kTextureWrapModes{
    std::tuple{ "GL_REPEAT", GL_REPEAT },
    std::tuple{ "GL_MIRRORED_REPEAT", GL_MIRRORED_REPEAT },
    std::tuple{ "GL_CLAMP_TO_EDGE", GL_CLAMP_TO_EDGE },
    std::tuple{ "GL_CLAMP_TO_BORDER", GL_CLAMP_TO_BORDER },
  };

  inline static const std::array kTextureFilters{
    std::tuple{ "GL_NEAREST", GL_NEAREST },
    std::tuple{ "GL_LINEAR", GL_LINEAR },
    std::tuple{ "GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST },
    std::tuple{ "GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST },
    std::tuple{ "GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR },
    std::tuple{ "GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR },
  };

  int texture_wrap_s_ = 0;
  int texture_wrap_t_ = 0;
  int texture_min_filter_ = 5;
  int texture_mag_filter_ = 1;
};
