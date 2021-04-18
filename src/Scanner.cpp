#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <spdlog/spdlog.h>
#include <sys/select.h>
#include <unistd.h>

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
  socklen_t originalFilterLen;
  
  if (getsockopt(hcidev_, SOL_HCI, HCI_FILTER, &originalFilter, &originalFilterLen) < 0) {
    spdlog::warn("Cannot get HCI filter: {}", strerror(errno));
    return -1;
  }

  hci_filter_clear(&newFilter);
	hci_filter_set_ptype(HCI_EVENT_PKT, &newFilter);
	hci_filter_set_event(EVT_LE_META_EVENT, &newFilter);

  if (setsockopt(hcidev_, SOL_HCI, HCI_FILTER, &newFilter, sizeof(newFilter)) < 0) {
    spdlog::warn("Cannot set HCI filter: {}", strerror(errno));
    return -1;
  }


  struct timeval timeout;
  timeout.tv_sec = 5; // FIXME!! configurable!!
  timeout.tv_usec = 0;
  fd_set readFds;
  FD_ZERO(&readFds);
  FD_SET(hcidev_, &readFds);

  int rc;
  while ((rc = select(1, &readFds, nullptr, nullptr, &timeout)) > 0) {
    uint8_t buffer[HCI_MAX_EVENT_SIZE];
    ssize_t len;

    len = read(hcidev_, buffer, sizeof(buffer));

    if (len < 0) {
      if (errno == EINTR || errno == EAGAIN) {
        continue;
      }

      spdlog::warn("read() from HCI device failed: {}", strerror(errno));
      // Might as well keep trying I guess?
      continue;
    }

    assert(len > HCI_EVENT_HDR_SIZE + sizeof(evt_le_meta_event));
 
    evt_le_meta_event *meta = (evt_le_meta_event*)&buffer[1 + HCI_EVENT_HDR_SIZE];

    if (meta->subevent != EVT_LE_ADVERTISING_REPORT) {
      continue;
    }
 
    le_advertising_info *info = (le_advertising_info *) (meta->data + 1);

      char addr[18];
      char name[30];

      memset(name, 0, sizeof(name));

      ba2str(&info->bdaddr, addr);

      printf("I see %s!!\n", addr);
    
  }

  if (rc < 0) {
    spdlog::warn("select() failed: {}", strerror(errno));
  }

  

  if (setsockopt(hcidev_, SOL_HCI, HCI_FILTER, &originalFilter, originalFilterLen) < 0) {
    spdlog::warn("Cannot restore HCI filter: {}", strerror(errno));
  }


  return rc; 
}


#ifdef SCANNER_TEST
int main(void) {
  Scanner scanner;
  scanner.scan();
}
#endif

