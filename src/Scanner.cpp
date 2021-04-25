#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <spdlog/spdlog.h>
#include <sys/select.h>
#include <unistd.h>

#include <cstring>

#include "Scanner.h"

Scanner::Scanner(std::vector<std::string> const &blessedDevices,
                 unsigned timeoutSeconds)
    : timeoutSeconds_(timeoutSeconds), terminating_(false) {

  for (const auto &addressStr : blessedDevices) {
    bdaddr_t addr;

    if (str2ba(addressStr.c_str(), &addr)) {
      spdlog::error("Invalid bluetooth device address {}", addressStr);
      throw std::runtime_error("Invalid bluetooth device address.");
    }

    blessedDevices_.push_back(addr);

    spdlog::debug("Registered blessed device {}", addressStr);
  }

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
  stopScanning();
  if (hcidev_ >= 0) {
    hci_close_dev(hcidev_);
  }
}

void Scanner::disableScanning() {
  if (hci_le_set_scan_enable(
          /*dev_id=*/hcidev_,
          /*enable=*/0,
          /*filter_duplicates=*/0,
          /*to=*/0) < 0) {
    spdlog::debug("hci_le_set_scan_enable(0) failed: {}", strerror(errno));
  }
}

void Scanner::scanThread(EventQueue &eq) {
  const int timeoutMs =
      timeoutSeconds_ * 1000; // milliseconds.
                              // FIXME: what does this timeout even control??

  // If we crashed or something and scanning is left enabled, nothing
  // works until we disable it. So just unconditionally force it off
  // here. Ignore any errors
  disableScanning();

  // Now we can enable scanning.
  int rc = hci_le_set_scan_parameters(
      /*dev_id=*/hcidev_,
      /*scan_type=*/0x01,         // ?? passive is 0, so I assume 1 is active?
      /*interval=*/htobs(0x0010), // ?? ripped from hcitool.
      /*window=*/htobs(0x0010),   // ?? ripped from hcitool.
      /*own_type=*/LE_PUBLIC_ADDRESS, // LE_RANDOM_ADDRESS is alternative.
      /*filter=*/0x00,                // ?? 1 is "Whitelist"
      /*to=*//*timeoutMs*/ 0);
  if (rc < 0) {
    spdlog::warn("hci_le_set_scan_parameters failed: {}", strerror(errno));
    return;
  }
  rc = hci_le_set_scan_enable(
      /*dev_id=*/hcidev_,
      /*enable=*/1,
      /*filter_duplicates=*/0,
      /*to=*/timeoutMs);
  if (rc < 0) {
    spdlog::warn("hci_le_set_scan_enable(1) failed: {}", strerror(errno));
    return;
  }

  spdlog::info("Scanning for BLE devices...");

  rc = checkAdvertisingDevices(eq);

  spdlog::info("Done scanning for BLE devices.");

  disableScanning();
}

int Scanner::checkAdvertisingDevices(EventQueue &eq) {
  static constexpr Event ndEvent{.type = Event::Type::NAZBERT_DETECTED};
  struct hci_filter originalFilter, newFilter;
  socklen_t originalFilterLen;
  int rc;

  if (getsockopt(hcidev_, SOL_HCI, HCI_FILTER, &originalFilter,
                 &originalFilterLen) < 0) {
    spdlog::warn("Cannot get HCI filter: {}", strerror(errno));
    return -1;
  }

  hci_filter_clear(&newFilter);
  hci_filter_set_ptype(HCI_EVENT_PKT, &newFilter);
  hci_filter_set_event(EVT_LE_META_EVENT, &newFilter);

  if (setsockopt(hcidev_, SOL_HCI, HCI_FILTER, &newFilter, sizeof(newFilter)) <
      0) {
    spdlog::warn("Cannot set HCI filter: {}", strerror(errno));
    return -1;
  }

  struct timeval timeout;
  timeout.tv_sec = 1; // ghetto timeout to wake and poll terminating flag.
  timeout.tv_usec = 0;
  fd_set readFds;
  FD_ZERO(&readFds);
  FD_SET(hcidev_, &readFds);

  while ((rc = select(hcidev_ + 1, &readFds, nullptr, nullptr, &timeout)) >=
         0) {
    uint8_t buffer[HCI_MAX_EVENT_SIZE];
    ssize_t len, needed = 0;

    if (terminating_) {
      break;
    }

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if (!rc) {
      continue;
    }

    len = read(hcidev_, buffer, sizeof(buffer));

    if (len < 0) {
      if (errno == EINTR || errno == EAGAIN) {
        continue;
      }

      spdlog::warn("read() from HCI device failed: {}", strerror(errno));
      // Might as well keep trying I guess?
      continue;
    }

    // Parsing code optimized for sanity checking and readability.
    // It would be more efficient to make sure that we had enough data
    // for a type + hci_event_hdr + evt_le_meta_event + le_advertising_info
    // up front.

    // The first byte is a packet type. Doesn't seem to be an associated
    // structure in bluetooth headers, which I guess is OK since it is just
    // a byte.

    needed = 1;
    if (len < needed) {
      spdlog::warn("Read short packet from HCI device: got {}, needed {} for "
                   "packet type",
                   len, needed);
      continue;
    }

    const uint8_t *type = buffer;
    if (*type != HCI_EVENT_PKT) {
      spdlog::info("Got non-packet type {} from HCI device.", *type);
      continue;
    }

    needed += HCI_EVENT_HDR_SIZE;
    if (len < needed) {
      spdlog::warn("Read short packet from HCI device: got {}, needed {} for "
                   "hc_event_hdr",
                   len, needed);
      continue;
    }

    const hci_event_hdr *event_hdr = (hci_event_hdr *)(type + 1);
    if (event_hdr->evt != EVT_LE_META_EVENT) {
      spdlog::info("Got non-meta event from HCI device: {}", event_hdr->evt);
      continue;
    }

    needed += EVT_LE_META_EVENT_SIZE;
    if (len < needed) {
      spdlog::warn("Read short packet{} from HCI device, got {}, needed {} for "
                   "evt_le_meta_event.",
                   len, needed);
      continue;
    }

    const evt_le_meta_event *meta = (evt_le_meta_event *)(event_hdr + 1);
    if (meta->subevent != EVT_LE_ADVERTISING_REPORT) {
      spdlog::info("Got non-advertising report meta event {}", meta->subevent);
      continue;
    }

    // There is a single byte following the evt_le_meta_event which is the
    // number of following le_advertising_info structures.
    needed += 1;
    if (len < needed) {
      spdlog::warn("Read short packet{} from HCI device, got {}, needed {} for "
                   "le_advertising_info count.",
                   len, needed);
      continue;
    }

    const uint8_t *numReports = (uint8_t *)(meta + 1);
    const uint8_t *nextReport = (numReports + 1);
    for (auto i = 0; i < *numReports; ++i) {
      needed += LE_ADVERTISING_INFO_SIZE;
      if (len < needed) {
        spdlog::warn(
            "Read short packet{} from HCI device, got {}, needed {} for "
            "le_advertising_info #{} header.",
            len, needed, i);
        break;
      }
      const le_advertising_info *info = (le_advertising_info *)nextReport;
      needed += info->length + 1; // +1 for trailing RSSI byte.
      if (len < needed) {
        spdlog::warn(
            "Read short packet{} from HCI device, got {}, needed {} for "
            "le_advertising_info #{} body with length {}.",
            len, needed, i, info->length);
        break;
      }

      const int8_t rssi = (int8_t)info->data[info->length];
      char addr[18];
      ba2str(&info->bdaddr, addr);

      // spdlog::debug("Device {} rssi {}.", addr, (int)rssi);

      for (const auto &bd : blessedDevices_) {
        if (!bacmp(&bd, &info->bdaddr)) {
          if (rssi > -70) { // FIXME: configurable!!
            spdlog::info("Blessed device {} is in range with RSSI {}", addr,
                         rssi);
            eq.send(ndEvent);
            break;
          }
        }
      }

      nextReport += LE_ADVERTISING_INFO_SIZE + info->length + 1;
    }
  }

  if (rc < 0) {
    spdlog::warn("select() failed: {}", strerror(errno));
  }

  if (setsockopt(hcidev_, SOL_HCI, HCI_FILTER, &originalFilter,
                 originalFilterLen) < 0) {
    spdlog::warn("Cannot restore HCI filter: {}", strerror(errno));
  }

  return rc;
}

int Scanner::startScanning(EventQueue &eq) {
  if (scanThread_.joinable()) {
    spdlog::warn("Scanner already running.");
    return -EBUSY;
  }

  terminating_ = false;
  scanThread_ = std::thread([&eq, this] { this->scanThread(eq); });
  return 0;
}

int Scanner::stopScanning() {
  if (scanThread_.joinable()) {
    terminating_ = true;
    scanThread_.join();
  }
  return 0;
}

#ifdef SCANNER_TEST
#include <iostream>
int main(void) {
  // spdlog::set_level(spdlog::level::debug);

  std::vector<std::string> blessedDevices;

  blessedDevices.push_back("F1:15:32:5B:7E:66");

  Scanner scanner(blessedDevices);
  EventQueue eq;

  while (1) {
    scanner.startScanning(eq);
    bool gotNaz = false;

    while (!gotNaz) {
      Event e = eq.wait();
      switch (e.type) {
        case Event::Type::NAZBERT_DETECTED:
          std::cout << "Nazbert detected!\n";
          gotNaz = true;
          break;
        default:
          std::cout << "WTF??\n";
          break;
      }
    }

    scanner.stopScanning();
  }
}
#endif
