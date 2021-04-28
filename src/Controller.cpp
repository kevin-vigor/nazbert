#include "Controller.h"
#include <spdlog/spdlog.h>
#include <sys/stat.h>
#include <sys/types.h>

static constexpr const char *gFifo = "/dev/shm/pounceblat.fifo";

Controller::Controller() : terminating_(false) {
  if (mkfifo(gFifo, 0666) != 0) {
    if (errno != EEXIST) {
      spdlog::error("Cannot construct control fifo {}: {}", gFifo,
                    strerror(errno));
      throw std::runtime_error("Cannot construct control fifo.");
    }
  }
}

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
    int outFd = open(gFifo, O_WRONLY);
    const char cmd = 'T';
    if (outFd == -1) {
      spdlog::warn("Cannot open {} in controller shutdown: {}", gFifo,
                   strerror(errno));
    }
    if (write(outFd, &cmd, 1) != 1) {
      spdlog::warn("Cannot write {} in controller shutdown: {}", gFifo,
                   strerror(errno));
    }
    if (close(outFd)) {
      spdlog::warn("Cannot close {} in controller shutdown: {}", gFifo,
                   strerror(errno));
    }
    controlThread_.join();
  }
}

void Controller::controlThread(EventQueue &eq) {
  static constexpr Event enableEvent{.type = Event::Type::ENABLE};
  static constexpr Event disableEvent{.type = Event::Type::DISABLE};
  int rc;

  while (!terminating_) {
    int inFd = open(gFifo, O_RDONLY);

    if (inFd == -1) {
      spdlog::error("Error opening {}: {}", gFifo, strerror(errno));
      break;
    }

    char c;
    while ((rc = read(inFd, &c, 1)) == 1) {
      switch (c) {
        case 'E':
          spdlog::info("Controller received enable request.");
          eq.send(enableEvent);
          break;
        case 'D':
          spdlog::info("Controller received disable request.");
          eq.send(disableEvent);
          break;
        default:
          spdlog::warn("Controller received unknown request {}.", c);
          break;
      }
    }

    if (rc != 0) {
      spdlog::error("read({}) failed: {}", gFifo, strerror(errno));
      break;
    }

    close(inFd);
  }

  spdlog::info("Controller loop terminated.");
}

#ifdef CONTROL_TEST
int main(void) {
  spdlog::set_level(spdlog::level::debug);

  Controller controller;
  EventQueue eq;

  controller.run(eq);

  spdlog::info("Waiting for events...");
  while (1) {
    Event e = eq.wait();
    spdlog::info("Got a {} event", e);
  }

  return 0;
}
#endif
