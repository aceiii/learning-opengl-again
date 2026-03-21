#include <print>
#include <quill/SimpleSetup.h>
#include <quill/LogFunctions.h>
#include "application.hpp"


auto main() -> int {
  quill::Logger* logger = quill::simple_logger();

  Application app;
  if (auto res = app.Init(); !res.has_value()) {
    quill::error(logger, "Failed to initialize app: {}", res.error());
    app.Cleanup();
    return -1;
  }

  app.Run();
  app.Cleanup();

  return 0;
}

