/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include "roq/server/flags/settings.hpp"

#include "roq/deribit/flags/fix.hpp"
#include "roq/deribit/flags/flags.hpp"
#include "roq/deribit/flags/mbp.hpp"
#include "roq/deribit/flags/misc.hpp"
#include "roq/deribit/flags/multicast.hpp"
#include "roq/deribit/flags/request.hpp"
#include "roq/deribit/flags/rest.hpp"
#include "roq/deribit/flags/ws.hpp"

namespace roq {
namespace deribit {

struct Settings final : public server::flags::Settings, public flags::Flags {
  explicit Settings(args::Parser const &);

  flags::Misc misc;
  flags::FIX fix;
  flags::WS ws;
  flags::Multicast multicast;
  flags::MBP mbp;
  flags::Request request;
  flags::REST rest;
};

}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::Settings> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::deribit::Settings const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(misc={}, )"
        R"(fix={}, )"
        R"(ws={}, )"
        R"(multicast={}, )"
        R"(mbp={}, )"
        R"(request={}, )"
        R"(rest={}, )"
        R"(server={})"
        R"(}})"sv,
        value.misc,
        value.fix,
        value.ws,
        value.multicast,
        value.mbp,
        value.request,
        value.rest,
        static_cast<roq::server::Settings const &>(value));
  }
};
