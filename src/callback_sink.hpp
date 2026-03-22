#pragma once

#include <functional>
#include <print>
#include <string>
#include <string_view>
#include <quill/sinks/Sink.h>


namespace Logger {

  class CallbackSink final : public quill::Sink {
  public:
    using Callback = std::function<void(std::string_view)>;

    CallbackSink() = default;

    void write_log(quill::MacroMetadata const* /** log_metadata **/, uint64_t /** log_timestamp **/,
                  std::string_view /** thread_id **/, std::string_view /** thread_name **/,
                  std::string const& /** process_id **/, std::string_view /** logger_name **/,
                  quill::LogLevel /** log_level **/, std::string_view /** log_level_description **/,
                  std::string_view /** log_level_short_code **/,
                  std::vector<std::pair<std::string, std::string>> const* /** named_args - only populated when named args in the format placeholder are used **/,
                  std::string_view /** log_message **/, std::string_view log_statement) override
    {
      cached_log_statements_.push_back(std::string{log_statement.data(), log_statement.size()});
    }

    void flush_sink() noexcept override {
      for (const auto& message : cached_log_statements_) {
        for (const auto& pair : callbacks_) {
          pair.second(message);
        }
      }
      cached_log_statements_.clear();
    }

    void run_periodic_tasks() noexcept override {}

    uint32_t AddCallback(Callback callback) {
      uint32_t callback_id = next_callback_id_++;
      callbacks_.push_back({callback_id, std::move(callback)});
      return callback_id;
    }

    void RemoveCallback(uint32_t callback_id) {
      std::erase_if(callbacks_, [=](const auto& pair) {
        return pair.first == callback_id;
      });
    }

    void ClearCallbacks() {
      callbacks_.clear();
    }

  private:
    inline static uint32_t next_callback_id_ = 0;
    std::vector<std::string> cached_log_statements_;
    std::vector<std::pair<uint32_t, Callback>> callbacks_;
  };

}
