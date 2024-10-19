/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/deribit/pcap_dump/application.hpp"

#include "roq/logging.hpp"

#include "roq/deribit/pcap_dump/controller.hpp"
#include "roq/deribit/pcap_dump/settings.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {
namespace pcap_dump {

// === IMPLEMENTATION ===

int Application::main(args::Parser const &args) {
  Settings settings{args};
  auto params = args.params();
  if (std::size(params) != 1)
    log::fatal("Expected exactly one argument"sv);
  Controller{settings, params[0]}.dispatch();
  return EXIT_SUCCESS;
}

}  // namespace pcap_dump
}  // namespace deribit
}  // namespace roq
