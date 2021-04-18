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

void Scanner::disableScanning() {
  if (hci_le_set_scan_enable(
          /*dev_id=*/hcidev_,
          /*enable=*/0,
          /*filter_duplicates=*/1,
          /*to=*/0) < 0) {
    // log but ignore errors: if we can enable it again later, that's cool.
    spdlog::warn("hci_le_set_scan_enable(0) failed: {}", strerror(errno));
  }
}

int Scanner::scan() {
  constexpr int timeout = 5000; // milliseconds, I think?
                                // FIXME: what does this timeout even control??

  // If we crashed or something and scanning is left enabled, nothing
  // works until we disable it. So just unconditionally force it off
  // here. Ignore any errors
  disableScanning();

  // Now we can enable scanning.
  int rc = hci_le_set_scan_enable(
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

  disableScanning();

  return 0;
}

int Scanner::checkAdvertisingDevices() {
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
  timeout.tv_sec = 10; // FIXME!! Make configurable!!
  timeout.tv_usec = 0;
  fd_set readFds;
  FD_ZERO(&readFds);
  FD_SET(hcidev_, &readFds);

  while ((rc = select(hcidev_ + 1, &readFds, nullptr, nullptr, &timeout)) > 0) {
    uint8_t buffer[HCI_MAX_EVENT_SIZE];
    ssize_t len, needed = 0;

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

    needed += LE_ADVERTISING_INFO_SIZE + 1;
    if (len < needed) {
      spdlog::warn("Read short packet{} from HCI device, got {}, needed {} for "
                   "le_advertising_info.",
                   len, needed);
      continue;
    }

    // There is a mystery byte between evt_le_meta_event and
    // le_advertising_info, I have no idea  what it is. Hence the "meta->data +
    // 1" rather than "meta + 1".
    const le_advertising_info *info = (le_advertising_info *)(meta->data + 1);

    // Can't find any documentation to support this, but the bluez
    // ./tools/parser/hci.c code and experiment show that the RSSI is
    // always in a single byte trailing the variable length advertising info.
    needed += info->length + 1;
    if (len < needed) {
      spdlog::warn("Read short packet from HCI device, got {}, needed {} for "
                   "trailing RSSI (adv len = {}).",
                   len, needed, info->length);
      continue;
    }

    const int8_t rssi = (int8_t)info->data[info->length];

    spdlog::info("Need len >= {} got {} rssi byte {} mystery byte {}", needed,
                 len, info->data[info->length], meta->data[0]);

    char addr[18];
    ba2str(&info->bdaddr, addr);

    spdlog::info("Device {} rssi {}.\n", addr, (int)rssi);
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

#ifdef SCANNER_TEST
int main(void) {
  Scanner scanner;
  scanner.scan();
}
#endif
