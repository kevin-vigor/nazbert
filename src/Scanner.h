#pragma once

#include <bluetooth/bluetooth.h>
#include <string>
#include <vector>

#include "EventQueue.h"

class Scanner {
public:
  explicit Scanner(std::vector<std::string> const &blessedDevices,
                   unsigned timeoutSeconds = 5);
  ~Scanner();

  int startScanning(EventQueue &); // Spin up a thread to scan for blessed
                                   // devices and post events when detected.
  int stopScanning(); // Kill the scan thread (synchronously, it is dead when
                      // this fn returns). either use an eventfs to wake up the
                      // select() in scanning thread, or use ghetto 1 sec
                      // timeout on select() and check terminating flag "trick".

private:
  int hcidev_;
  int checkAdvertisingDevices(EventQueue &);
  void disableScanning();
  void scanThread(EventQueue &);

  std::vector<bdaddr_t> blessedDevices_;
  unsigned timeoutSeconds_;

  std::thread scanThread_;
  bool terminating_;
};
