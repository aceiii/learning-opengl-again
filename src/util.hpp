#pragma once

namespace Util {

  float Lerp(float a, float b, float f) {
    return a + f * (b - a);
  }
}
