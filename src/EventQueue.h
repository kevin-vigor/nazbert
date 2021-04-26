#pragma once

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <spdlog/spdlog.h>

class Event {
public:
  enum class Type { DISABLE, ENABLE, MOTION_DETECTED, NAZBERT_DETECTED, TIMEOUT } type;
};

class EventQueue {
public:
  EventQueue() {}
  ~EventQueue() {}

  void send(Event const &);
  Event wait();

  void setTimeout(std::chrono::milliseconds delay) {
    deadline_ = std::chrono::steady_clock::now() + delay;
  }
  void clearTimeout() { deadline_ = std::nullopt; }

private:
  std::deque<Event> queue_;
  std::mutex lock_;
  std::condition_variable cv_;
  std::optional<std::chrono::time_point<std::chrono::steady_clock>> deadline_;
};

template <> struct fmt::formatter<Event> {
  constexpr auto parse(format_parse_context &ctx) {
    auto it = ctx.begin();
    if (*it != '}') {
      throw format_error("invalid format");
    }
    return it;
  }

  template <typename FormatContext>
  auto format(const Event &e, FormatContext &ctx) {
    const char *name = "bogus";
    switch (e.type) {
      case Event::Type::DISABLE:
        name = "DISABLE";
        break;
      case Event::Type::ENABLE:
        name = "ENABLE";
        break;
      case Event::Type::MOTION_DETECTED:
        name = "MOTION_DETECTED";
        break;
      case Event::Type::TIMEOUT:
        name = "TIMEOUT";
        break;
      case Event::Type::NAZBERT_DETECTED:
        name = "NAZBERT_DETECTED";
        break;
    }
    return format_to(ctx.out(), "{}", name);
  }
};
