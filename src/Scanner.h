#pragma once

class Scanner {
public:
  Scanner();
  ~Scanner();

  int scan();

private:
  int hcidev_;
  int checkAdvertisingDevices();
  void disableScanning();
};
