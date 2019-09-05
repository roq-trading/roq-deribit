/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <cstddef>
#include <string_view>
#include <vector>

#include "roq/core/fix/reader.h"

#include "roq/deribit/fix/instrument.h"

namespace roq {
namespace deribit {
namespace fix {

struct SecurityList final {
  std::string_view security_req_id;
  core::fix::SecurityRequestResult security_request_result = core::fix::SecurityRequestResult::UNKNOWN;
  std::string_view security_response_id;

  struct {
    Instrument *items = nullptr;
    size_t length = 0;
  } instruments;  // Instrument

  static SecurityList parse(
      const core::fix::message_t& message,
      std::vector<std::byte>& buffer);

  static void parse(
      SecurityList&,
      const core::fix::message_t& message,
      std::vector<std::byte>& buffer);

  void parse(
      core::fix::message_t::const_iterator&& iter,
      const core::fix::message_t::const_iterator& end,
      std::vector<std::byte>& buffer);
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::SecurityList> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::SecurityList& value, C& ctx) {
    return format_to(
        ctx.begin(),
        "{{"
        "security_req_id=\"{}\", "
        "security_request_result={}, "
        "security_response_id=\"{}\", "
        "instruments=[{}]"
        "}}",
        value.security_req_id,
        value.security_request_result,
        value.security_response_id,
        fmt::join(
            value.instruments.items,
            value.instruments.items + value.instruments.length,
            ", "));
  }
};
