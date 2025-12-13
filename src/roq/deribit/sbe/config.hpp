/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <cstdint>
#include <string>
#include <string_view>

#include "roq/utils/container.hpp"

namespace roq {
namespace deribit {
namespace sbe {

struct Config final {
  struct Connection final {
    std::string currency_product;
    std::string address;
  };

  Config(std::string_view const &filename, std::vector<std::string> const &channel_ids);

  auto events_port() const { return ports_.first; }
  auto snapshot_port() const { return ports_.second; }

  template <typename Callback>
  void get_connections(Callback callback) const {
    for (auto &[_, connection] : connections_) {
      callback(connection);
    }
  }

 private:
  std::pair<uint16_t, uint16_t> const ports_;  // events, snapshot
  utils::unordered_map<std::string, Connection> const connections_;
};

}  // namespace sbe
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::sbe::Config::Connection> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::deribit::sbe::Config::Connection const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(curency_product="{}", )"
        R"(address="{}")"
        R"(}})"sv,
        value.currency_product,
        value.address);
  }
};

template <>
struct fmt::formatter<roq::deribit::sbe::Config> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::deribit::sbe::Config const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(ports={{)"
        R"(events={}, )"
        R"(snapshot={})"
        R"(}}, )"
        R"(connections=[...])"
        R"(}})"sv,
        value.events_port(),
        value.snapshot_port());
  }
};
