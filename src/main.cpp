#include "PounceBlat.h"

#include <getopt.h>
#include <spdlog/spdlog.h>
#include <unistd.h>

static constexpr struct option long_options[] = {
    {"debug", no_argument, NULL, 'd'},
    {nullptr, 0, nullptr, 0},
};

int main(int argc, char *argv[]) {
  int ch;
  bool debug = false;

  while ((ch = getopt_long(argc, argv, "d", long_options, nullptr)) != -1) {
    switch (ch) {
      case 'd':
        debug = true;
        break;
    }
  }

  if (debug) {
    spdlog::set_level(spdlog::level::debug);
  }

  spdlog::info("Here starts blatting!");

  PounceBlat blatter;

  blatter.run();

  return 0;
}
