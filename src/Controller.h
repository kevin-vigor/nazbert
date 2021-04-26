#pragma once

#include "EventQueue.h"

class Controller {
public:
  Controller();
  ~Controller();

  void run(EventQueue &);

private:
};
