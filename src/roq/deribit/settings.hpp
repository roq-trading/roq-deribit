/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <fmt/compile.h>
#include <fmt/format.h>

#include "roq/server/flags/settings.hpp"

#include "roq/deribit/flags/common.hpp"
#include "roq/deribit/flags/fix.hpp"
#include "roq/deribit/flags/flags.hpp"
#include "roq/deribit/flags/multicast.hpp"
#include "roq/deribit/flags/ws.hpp"

namespace roq {
namespace deribit {

struct Settings final : public server::flags::Settings, public flags::Flags {
  explicit Settings(args::Parser const &);

  flags::Common common;
  flags::FIX fix;
  flags::WS ws;
  flags::Multicast multicast;
};

}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::Settings> {
  template <typename Context>
  constexpr auto parse(Context &context) {
    return std::begin(context);
  }
  template <typename Context>
  auto format(roq::deribit::Settings const &value, Context &context) const {
    using namespace fmt::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(common={}, )"
        R"(fix={}, )"
        R"(ws={}, )"
        R"(multicast={}, )"
        R"(server={})"
        R"(}})"_cf,
        value.common,
        value.fix,
        value.ws,
        value.multicast,
        static_cast<roq::server::Settings const &>(value));
  }
};
