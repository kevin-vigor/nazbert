#pragma once

class Relay {
public:
  Relay();
  ~Relay();
  int set(bool enable);

private:
  int fd_;
};
