#include <vector>
#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/sinks/ConsoleSink.h>

#include "logger.hpp"
#include "callback_sink.hpp"


namespace {
  const std::string kLoggerName = "root_logger";
  const std::string kConsoleSinkName = "console_sink";
  const std::string kCallbackSinkName = "callback_sink";
}

quill::Logger* Logger::GetRootLogger() {
  quill::Logger* logger = quill::Frontend::get_logger(kLoggerName);

  if (logger) {
    return logger;
  }

  quill::BackendOptions backend_options{};
  quill::Backend::start(backend_options);

  const quill::PatternFormatterOptions pattern_formatter_options{
    "[%(time)] [%(log_level)] %(message)"
  };

  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>(kConsoleSinkName);
  auto callback_sink = quill::Frontend::create_or_get_sink<CallbackSink>(kCallbackSinkName);

  logger = quill::Frontend::create_or_get_logger(kLoggerName, {std::move(console_sink), std::move(callback_sink)}, pattern_formatter_options);

  return logger;
}

uint32_t Logger::AddLogCallback(Logger::LogCallback callback) {
  auto sink_ptr = quill::Frontend::create_or_get_sink<CallbackSink>(kCallbackSinkName);
  auto callback_sink = std::dynamic_pointer_cast<CallbackSink>(sink_ptr);
  return callback_sink->AddCallback(callback);
}

void Logger::RemoveLogCallback(uint32_t id) {
  auto sink_ptr = quill::Frontend::create_or_get_sink<CallbackSink>(kCallbackSinkName);
  auto callback_sink = std::dynamic_pointer_cast<CallbackSink>(sink_ptr);
  callback_sink->RemoveCallback(id);
}

void Logger::ClearCallbacks() {
  auto sink_ptr = quill::Frontend::create_or_get_sink<CallbackSink>(kCallbackSinkName);
  auto callback_sink = std::dynamic_pointer_cast<CallbackSink>(sink_ptr);
  callback_sink->ClearCallbacks();
}
