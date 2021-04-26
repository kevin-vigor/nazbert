#include "Controller.h"
#include <spdlog/spdlog.h>

Controller::Controller() : terminating_(false) {}

Controller::~Controller() {}

void Controller::run(EventQueue &eq) {
  if (controlThread_.joinable()) {
    spdlog::warn("Already running.");
    return;
  }

  controlThread_ = std::thread([&eq, this] { this->controlThread(eq); });
}

void Controller::stop() {
  if (controlThread_.joinable()) {
    terminating_ = true;
    controlThread_.join();
  }
}

void Controller::controlThread(EventQueue &eq) {
  static constexpr Event ndEvent{.type = Event::Type::NAZBERT_DETECTED};
  while (!terminating_) {
    eq.send(ndEvent);
    ::sleep(1);
  }
}

#ifdef CONTROL_TEST
#include <iostream>
int main(void) {

  Controller controller;
  EventQueue eq;

  controller.run(eq);

  while (1) {
    Event e = eq.wait();
    spdlog::info("Got a {} event", e);
  }

  return 0;
}
#endif
