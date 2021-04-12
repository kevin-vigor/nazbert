#pragma once

#include "EventQueue.h"

#include <gpiod.hpp>

#include <thread>

class Sensor {
public:
  Sensor();
  ~Sensor();

  void monitor(EventQueue &);

private:
  std::thread monitorThread_;
  bool terminating_;
  ::gpiod::chip chip_;
  ::gpiod::line line_;
};
