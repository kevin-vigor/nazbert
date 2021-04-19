#pragma once

#include "EventQueue.h"
#include "Relay.h"
#include "Scanner.h"
#include "Sensor.h"

class PounceBlat {
public:
  explicit PounceBlat(std::vector<std::string> blessedDevices);
  void run();

  enum class State { ARMED, SCANNING, RUNNING, GRACE };

private:
  Relay relay_;
  Sensor sensor_;
  EventQueue eq_;
  Scanner scanner_;

  State state_;

  void transitionTo(State s);
};

template <> struct fmt::formatter<PounceBlat::State> {
  constexpr auto parse(format_parse_context &ctx) {
    auto it = ctx.begin();
    if (*it != '}') {
      throw format_error("invalid format");
    }
    return it;
  }

  template <typename FormatContext>
  auto format(const PounceBlat::State &s, FormatContext &ctx) {
    const char *name = "bogus";
    switch (s) {
      case PounceBlat::State::ARMED:
        name = "ARMED";
        break;
      case PounceBlat::State::SCANNING:
        name = "SCANNING";
        break;
      case PounceBlat::State::RUNNING:
        name = "RUNNING";
        break;
      case PounceBlat::State::GRACE:
        name = "GRACE";
        break;
    }
    return format_to(ctx.out(), "{}", name);
  }
};
