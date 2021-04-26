#include "Controller.h"
#include <spdlog/spdlog.h>

Controller::Controller() {}

Controller::~Controller() {}

void Controller::run(EventQueue &eq) {
  // FIXME!
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
