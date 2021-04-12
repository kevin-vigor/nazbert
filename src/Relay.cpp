#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <unistd.h>

#include "Relay.h"

Relay::Relay() {
  fd_ = open("/dev/i2c-1", O_RDWR);
  if (fd_ == -1) {
    perror("Opening i2c device for relay");
    throw std::runtime_error("opening relay");
  }
  if (ioctl(fd_, I2C_SLAVE, 0x10) < 0) {
    perror("ioctl(I2C_SLAVE)");
    throw std::runtime_error("initializing relay");
  }
}

Relay::~Relay() { close(fd_); }

int Relay::set(bool enabled) {
  uint8_t buf[2];
  buf[0] = 1; // register, i.e. relay number 1-4
  buf[1] = enabled ? 0xff : 0;
  if (write(fd_, buf, 2) != 2) {
    perror("I2C write failed");
    return -1;
  }
  return 0;
}

#ifdef RELAY_TEST
int main(void) {
  Relay r;
  if (r.set(true)) {
    return 1;
  }
  sleep(3);
  if (r.set(false)) {
    return 1;
  }
  return 0;
}
#endif
