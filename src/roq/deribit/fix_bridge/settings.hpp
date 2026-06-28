/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/server/fix_bridge/settings.hpp"

#include "roq/deribit/flags/settings.hpp"

namespace roq {
namespace deribit {
namespace fix_bridge {

struct Settings final : public flags::Settings {
  explicit Settings(args::Parser const &);

  server::fix_bridge::Settings fix_bridge;
};

}  // namespace fix_bridge
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix_bridge::Settings> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::deribit::fix_bridge::Settings const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(gateway={}, )"
        R"(fix_bridge={})"
        R"(}})"sv,
        static_cast<roq::deribit::flags::Settings const &>(value),
        value.fix_bridge);
  }
};
