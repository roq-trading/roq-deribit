/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <fmt/chrono.h>
#include <fmt/compile.h>
#include <fmt/format.h>

#include "roq/server/flags/settings.hpp"

#include "roq/deribit/flags/common.hpp"
#include "roq/deribit/flags/fix.hpp"
#include "roq/deribit/flags/multicast.hpp"
#include "roq/deribit/flags/ws.hpp"

namespace roq {
namespace deribit {

struct Settings final : public server::flags::Settings {
  explicit Settings(args::Parser const &, server::Type);

  std::string_view exchange;

  flags::Common__flags common;
  flags::FIX__flags fix;
  flags::WS__flags ws;
  flags::Multicast__flags multicast;
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
        R"(exchange="{}", )"
        R"(common={}, )"
        R"(fix={}, )"
        R"(ws={}, )"
        R"(multicast={}, )"
        R"(server={})"
        R"(}})"_cf,
        value.exchange,
        value.common,
        value.fix,
        value.ws,
        value.multicast,
        static_cast<roq::server::Settings const &>(value));
  }
};
