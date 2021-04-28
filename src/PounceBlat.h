#pragma once

#include "Controller.h"
#include "EventQueue.h"
#include "Relay.h"
#include "Scanner.h"
#include "Sensor.h"

struct BlatStats {
  unsigned motion = 0;
  unsigned runs = 0;
  unsigned disallowed = 0;
  unsigned aborts = 0;
};

class PounceBlat {
public:
  explicit PounceBlat(std::vector<std::string> blessedDevices);
  void run();

  enum class State { ARMED, DISABLED, GRACE, RUNNING, SCANNING };
  static const char *stateName(State s);

private:
  Relay relay_;
  Sensor sensor_;
  EventQueue eq_;
  Scanner scanner_;
  Controller controller_;

  State state_;

  BlatStats stats_;

  void transitionTo(State s);
  void publishStats();
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
    return format_to(ctx.out(), "{}", PounceBlat::stateName(s));
  }
};
