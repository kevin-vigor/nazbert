#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <spdlog/spdlog.h>

#include <cstring>

#include "Scanner.h"

Scanner::Scanner() {
  // Get the default HCI device. If we had more than one,
  // this would have to be more clever.
  int dev_id = hci_get_route(NULL);
  hcidev_ = hci_open_dev(dev_id);
  if (hcidev_ < 0) {
    spdlog::warn("Cannot open default HCI device: {}", strerror(errno));
    throw std::runtime_error("Scanner initialization failed.");
  }
}

Scanner::~Scanner() {
  if (hcidev_ >= 0) {
    hci_close_dev(hcidev_);
  }
}

int Scanner::scan() {
  constexpr int timeout = 5000;     // milliseconds, I think?
  int rc = hci_le_set_scan_parameters(
    /*dev_id=*/hcidev_,
    /*scan_type=*/0x01,             // ?? passive is 0, so I assume 1 is active?
    /*interval=*/htobs(0x0010),     // ?? ripped from hcitool.
    /*window=*/htobs(0x0010),       // ?? ripped from hcitool.
    /*own_type=*/LE_PUBLIC_ADDRESS, // LE_RANDOM_ADDRESS is alternative.
    /*filter=*/0x00,                // ?? 1 is "Whitelist"
    /*to=*/timeout);
  if (rc < 0) {
    spdlog::warn("hci_le_set_scan_parameters failed: {}", strerror(errno));
    return rc;
  }
  rc = hci_le_set_scan_enable(
        /*dev_id=*/hcidev_,
        /*enable=*/1,
        /*filter_duplicates=*/1,
        /*to=*/timeout);
  if (rc < 0) {
    spdlog::warn("hci_le_set_scan_enable(1) failed: {}", strerror(errno));
    return rc;
  }

  spdlog::info("Scanning for BLE devices...");

  rc = checkAdvertisingDevices();

  spdlog::info("Done scanning for BLE devices.");

  if (hci_le_set_scan_enable(
        /*dev_id=*/hcidev_,
        /*enable=*/0,
        /*filter_duplicates=*/1,
        /*to=*/timeout) < 0) {
    // log but ignore: failure here does not affect validity of results AFAIK.
    spdlog::warn("hci_le_set_scan_enable(0) failed: {}", strerror(errno));
  }

  return 0;
}

int Scanner::checkAdvertisingDevices() {
  struct hci_filter originalFilter, newFilter;
  socklen_t olen;
  
  if (getsockopt(hcidev_, SOL_HCI, HCI_FILTER, &originalFilter, &olen) < 0) {
    spdlog::warn("Cannot get HCI filter: {}", strerror(errno));
    return -1;
  }

  hci_filter_clear(&newFilter);


  return 0; 
}


#ifdef SCANNER_TEST
int main(void) {
  Scanner scanner;
  scanner.scan();
}
#endif

