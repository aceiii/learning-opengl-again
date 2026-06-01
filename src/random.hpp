#pragma once

#include <random>


namespace Random {
  class RandInt {
  public:
    RandInt(int min = INT32_MIN, int max = INT32_MAX): gen_{rd_()}, distrib_{min, max} {}

    int Next() {
      return distrib_(gen_);
    }

  private:
    std::random_device rd_;
    std::mt19937 gen_;
    std::uniform_int_distribution<int> distrib_;
  };

  class RandFloat {
  public:
    RandFloat(float min = 0.0f, float max = 1.0f): gen_{rd_()}, distrib_{min, max} {}

    int Next() {
      return distrib_(gen_);
    }

  private:
    std::random_device rd_;
    std::mt19937 gen_;
    std::uniform_real_distribution<float> distrib_;
  };
}
