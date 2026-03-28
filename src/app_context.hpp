#pragma once

#include <utility>

#include "key.hpp"
#include "mouse.hpp"


class IAppContext {
public:
  virtual ~IAppContext() = default;

  virtual float GetFrameTime() const = 0;
  virtual std::pair<int, int> GetWindowSize() const = 0;
  virtual std::pair<float, float> GetMousePosition() const = 0;
  virtual bool IsKeyDown(Key key) const = 0;
  virtual bool IsMouseButtonDown(Mouse mouse) const = 0;

  virtual void RequestQuit() = 0;
  virtual void CaptureCursor(bool enabled) = 0;
  virtual void ToggleUI(bool enabled) = 0;

};
