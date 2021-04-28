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
      case State::DISABLED:
        relay_.set(false);       // Should be a no-op... but can't hurt, eh?
        scanner_.stopScanning(); // likewise.
        break;
      case State::GRACE:
        relay_.set(false);
        scanner_.stopScanning();
        eq_.setTimeout(std::chrono::seconds(10));
        break;
      case State::RUNNING:
        // NB: we do *not* stop scanning on this transition, so that if Nazbert
        // wanders into range while running we detect it and halt ASAP.
        relay_.set(true);
        eq_.setTimeout(std::chrono::seconds(5)); // FIXME: configurable runtime?
        break;
      case State::SCANNING:
        scanner_.startScanning(eq_);
        eq_.setTimeout(std::chrono::seconds(5)); // FIXME: configurable runtime?
        break;
    }
    publishStats();
  } else {
    spdlog::warn(
        "Attempted to transition to state {} when already in that state.");
  }
}

void PounceBlat::run() {
  sensor_.monitor(eq_);
  controller_.run(eq_);

  publishStats();

  while (1) {
    Event e = eq_.wait();

    spdlog::debug("Received event {} in state {}", e, state_);

    switch (state_) {
      case State::ARMED:
        switch (e.type) {
          case Event::Type::DISABLE:
            transitionTo(State::DISABLED);
            break;
          case Event::Type::ENABLE:
            spdlog::warn("Enable ignored in {} state.", state_);
            break;
          case Event::Type::MOTION_DETECTED:
            spdlog::info("Motion detected!");
            stats_.motion++;
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

      case State::DISABLED:
        switch (e.type) {
          case Event::Type::ENABLE:
            transitionTo(State::ARMED);
            break;
          case Event::Type::DISABLE:
          case Event::Type::MOTION_DETECTED:
          case Event::Type::TIMEOUT:
          case Event::Type::NAZBERT_DETECTED:
            spdlog::debug("Event {} ignored in {} state.", e, state_);
            break;
        }
        break;

      case State::GRACE:
        switch (e.type) {
          case Event::Type::DISABLE:
            transitionTo(State::DISABLED);
            break;
          case Event::Type::ENABLE:
            spdlog::warn("Enable ignored in {} state.", state_);
            break;
          case Event::Type::MOTION_DETECTED:
          case Event::Type::NAZBERT_DETECTED:
            spdlog::debug("Event {} ignored in GRACE state.", e);
            break;
          case Event::Type::TIMEOUT:
            transitionTo(State::ARMED);
            break;
        }
        break;

      case State::RUNNING:
        switch (e.type) {
          case Event::Type::DISABLE:
            transitionTo(State::DISABLED);
            break;
          case Event::Type::ENABLE:
            spdlog::warn("Enable ignored in {} state.", state_);
            break;
          case Event::Type::MOTION_DETECTED:
            spdlog::debug("Motion ignored, already in RUNNING state.");
            break;
          case Event::Type::TIMEOUT:
            transitionTo(State::GRACE);
            break;
          case Event::Type::NAZBERT_DETECTED:
            spdlog::warn("Nazbert detected while running oh noes :(");
            stats_.aborts++;
            transitionTo(State::GRACE);
            break;
        }
        break;

      case State::SCANNING:
        switch (e.type) {
          case Event::Type::DISABLE:
            transitionTo(State::DISABLED);
            break;
          case Event::Type::ENABLE:
            spdlog::warn("Enable ignored in {} state.", state_);
            break;
          case Event::Type::MOTION_DETECTED:
            spdlog::info("Motion ignored in scanning state.");
            break;
          case Event::Type::TIMEOUT:
            spdlog::info("Scanning timed out, game on!");
            stats_.runs++;
            transitionTo(State::RUNNING);
            break;
          case Event::Type::NAZBERT_DETECTED:
            spdlog::warn(
                "Nazbert detected in SCANNING state, hold yer horses!");
            stats_.disallowed++;
            transitionTo(State::GRACE);
            break;
        }
        break;
    }
  }
}

const char *PounceBlat::stateName(State s) {
  switch (s) {
    case PounceBlat::State::ARMED:
      return "ARMED";
    case PounceBlat::State::DISABLED:
      return "DISABLED";
    case PounceBlat::State::GRACE:
      return "GRACE";
    case PounceBlat::State::RUNNING:
      return "RUNNING";
    case PounceBlat::State::SCANNING:
      return "SCANNING";
  }
  return "Impossible!";
}

void PounceBlat::publishStats() {
  char tmpName[] = "/dev/shm/pounceblat.status.XXXXXX";
  int tmpFd = mkstemp(tmpName);
  if (tmpFd == -1) {
    spdlog::warn("Cannot create temporary status file: {}", strerror(errno));
    return;
  }
  dprintf(tmpFd, "Current state: %s\n", stateName(state_));
  dprintf(tmpFd, "\n");
  dprintf(tmpFd, "Motion detected: %u\n", stats_.motion);
  dprintf(tmpFd, "Runs: %u\n", stats_.runs);
  dprintf(tmpFd, "Disallowed due to Nazbert: %u\n", stats_.disallowed);
  dprintf(tmpFd, "Aborted due to Nazbert: %u\n", stats_.aborts);

  if (close(tmpFd) == -1) {
    spdlog::warn("Error writing temporary status file: {}", strerror(errno));
    return;
  }

  if (rename(tmpName, "/dev/shm/pounceblat.status") == -1) {
    spdlog::warn("Error renaming temporary stats file: {}", strerror(errno));
  }
}
