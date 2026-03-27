#pragma once

#include "../scene.hpp"


class EmptyScene final : public Scene {
public:
  void Init() override {}

  void Update(float dt) override {}

  void Render() override {}

  void RenderInterface(int, int) override {}

  void ProcessInput(float dt, const SceneInputState& input) override {}

  void Cleanup() override {}

  virtual std::string Name() const override {
    return "<None>";
  }

  static std::shared_ptr<Scene> Get() {
    if (!instance_ptr_) {
      instance_ptr_ = std::make_shared<EmptyScene>();
    }
    return instance_ptr_;
  }

private:
  inline static std::shared_ptr<EmptyScene> instance_ptr_ = nullptr;
};
