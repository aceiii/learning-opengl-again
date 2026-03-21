#include <print>
#include "application.hpp"


auto main() -> int {
  Application app;
  if (auto res = app.Init(); !res.has_value()) {
    std::println(stderr, "Failed to initialize app: {}", res.error());
    app.Cleanup();
    return -1;
  }

  app.Run();
  app.Cleanup();

  return 0;
}

