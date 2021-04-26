#pragma once

#include "EventQueue.h"
#include <thread>

class Controller {
public:
  Controller();
  ~Controller();

  void run(EventQueue &);
  void stop();

private:
  void controlThread(EventQueue &);

  std::thread controlThread_;
  bool terminating_;
};
