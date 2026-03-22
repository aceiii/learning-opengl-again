#include <print>
#include "logger.hpp"
#include "application.hpp"


auto main() -> int {
  quill::Logger* logger = Logger::GetRootLogger();
  quill::info(logger, "[APPLICATION] Hello, world!");

  Application app;
  if (auto res = app.Init(); !res.has_value()) {
    quill::error(logger, "[APPLICATION] Failed to initialize app: {}", res.error());
    app.Cleanup();
    return -1;
  }
  app.Run();
  app.Cleanup();

  quill::info(logger, "[APPLICATION] Good bye");

  return 0;
}

