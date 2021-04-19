#include "PounceBlat.h"

#include <iostream>
#include <unistd.h>

#include <spdlog/spdlog.h>

PounceBlat::PounceBlat(std::vector<std::string> blessedDevices)
    : scanner_(blessedDevices), state_(State::ARMED) {}

void PounceBlat::transitionTo(State s) {
  if (s != state_) {
    spdlog::debug("Transition state from {} -> {}", state_, s);
    eq_.clearTimeout();
    state_ = s;
    switch (s) {
      case State::ARMED:
        relay_.set(false);       // Should be a no-op... but can't hurt, eh?
        scanner_.stopScanning(); // likewise.
        break;
      case State::SCANNING:
        scanner_.startScanning(eq_);
        eq_.setTimeout(std::chrono::seconds(5)); // FIXME: configurable runtime?
        break;
      case State::RUNNING:
        // NB: we do *not( stop scanning on this transition, so that if Nazbert
        // wanders into range while runnign we detect it and halt.
        relay_.set(true);
        eq_.setTimeout(std::chrono::seconds(5)); // FIXME: configurable runtime?
        break;
      case State::GRACE:
        relay_.set(false);
        scanner_.stopScanning();
        eq_.setTimeout(std::chrono::seconds(10));
        break;
    }
  } else {
    spdlog::warn(
        "Attempted to transition to state {} when already in that state.");
  }
}

void PounceBlat::run() {
  sensor_.monitor(eq_);

  while (1) {
    Event e = eq_.wait();

    spdlog::debug("Received event {} in state {}", e, state_);

    switch (state_) {
      case State::ARMED:
        switch (e.type) {
          case Event::Type::MOTION_DETECTED:
            spdlog::info("Motion detected!");
            transitionTo(State::SCANNING);
            break;

          case Event::Type::TIMEOUT:
            spdlog::warn("Unexpected timeout event in ARMED state.");
            break;

          case Event::Type::NAZBERT_DETECTED:
            spdlog::warn("Unexpected Nazbert in ARMED state.");
            transitionTo(State::GRACE);
            break;
        }
        break;

      case State::SCANNING:
        switch (e.type) {
          case Event::Type::MOTION_DETECTED:
            spdlog::info("Motion ignored in scanning state.");
            break;

          case Event::Type::TIMEOUT:
            spdlog::info("Scanning timed out, game on!");
            transitionTo(State::RUNNING);
            break;

          case Event::Type::NAZBERT_DETECTED:
            spdlog::warn(
                "Nazbert detected in SCANNING state, hold yer horses!");
            transitionTo(State::GRACE);
            break;
        }
        break;

      case State::RUNNING:
        switch (e.type) {
          case Event::Type::MOTION_DETECTED:
            spdlog::debug("Motion ignored, already in RUNNING state.");
            break;
          case Event::Type::TIMEOUT:
            transitionTo(State::GRACE);
            break;
          case Event::Type::NAZBERT_DETECTED:
            spdlog::warn("Nazbert detected while running oh noes :(");
            transitionTo(State::GRACE);
            break;
        }
        break;

      case State::GRACE:
        switch (e.type) {
          case Event::Type::MOTION_DETECTED:
          case Event::Type::NAZBERT_DETECTED:
            spdlog::debug("Event {} ignored in GRACE state.", e);
            break;
          case Event::Type::TIMEOUT:
            transitionTo(State::ARMED);
            break;
        }
        break;
    }
  }
}
