#pragma once

#include <expected>
#include <string>


class Scene {
public:
  using Result = std::expected<void, std::string>;

  virtual ~Scene() = default;

  virtual Result Init() = 0;
  virtual void Update(float dt) = 0;
  virtual void Render() = 0;
  virtual void RenderInterface(int window_width, int window_height) = 0;
  virtual void Cleanup() = 0;
  virtual std::string Name() const = 0;
};
