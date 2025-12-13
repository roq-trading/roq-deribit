/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/filter/controller.hpp"

#include <vector>

#include "roq/logging.hpp"

#include "roq/deribit/sbe/config.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {
namespace filter {

// === IMPLEMENTATION ===

Controller::Controller(Settings const &settings) : settings_{settings} {
  if (settings_.type != "tcpdump"sv) {
    log::fatal(R"(Unexpected: type="{}")"sv, settings_.type);
  }
}

void Controller::dispatch() {
  sbe::Config config{settings_.multicast.config_file, settings_.multicast.channel_ids};
  std::vector<std::string> filter;
  auto callback = [&](auto &connection) { filter.emplace_back(connection.address); };
  config.get_connections(callback);
  fmt::print(stdout, "(port {} or port {}) and ("sv, config.events_port(), config.snapshot_port());
  auto length = std::size(filter);
  for (size_t i = 0; i < length; ++i) {
    auto &item = filter[i];
    fmt::print(stdout, "host {}"sv, item);
    if (i != (length - 1)) {
      fmt::print(stdout, " or "sv);
    }
  }
  fmt::print(stdout, ")"sv);
}

}  // namespace filter
}  // namespace deribit
}  // namespace roq
