#include "PounceBlat.h"

#include "spdlog/sinks/rotating_file_sink.h"
#include <getopt.h>
#include <spdlog/spdlog.h>
#include <unistd.h>

static constexpr struct option long_options[] = {
    {"debug", no_argument, nullptr, 'd'},
    {"logfile", required_argument, nullptr, 'l'},
    {nullptr, 0, nullptr, 0},
};

int main(int argc, char *argv[]) {
  int ch;

  spdlog::flush_every(std::chrono::seconds(5));
  while ((ch = getopt_long(argc, argv, "dl:", long_options, nullptr)) != -1) {
    switch (ch) {
      case 'd':
        spdlog::set_level(spdlog::level::debug);
        break;
      case 'l':
        spdlog::set_default_logger(spdlog::rotating_logger_mt(
            "pounceblat", optarg, 16 * 1024 * 1024, 3));
        break;
    }
  }
  spdlog::info("Here starts blatting!");

  std::vector<std::string> blessedDevices;
  blessedDevices.push_back("F1:15:32:5B:7E:66");
  PounceBlat blatter(blessedDevices);

  blatter.run();

  return 0;
}
