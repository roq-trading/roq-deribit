/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <string_view>

#include "roq/core/fix/reader.h"

namespace roq {
namespace deribit {
namespace fix {

struct PositionReport final {
  std::string_view pos_maint_rpt_id;
  std::string_view pos_req_id;
  uint16_t pos_req_result = 0;  // TODO(thraneh): enum?
  uint16_t pos_req_type = 0;  // TODO(thraneh): enum?

  static PositionReport parse(const core::fix::message_t& message);
  static void parse(PositionReport&, const core::fix::message_t& message);

  void parse(
      core::fix::message_t::const_iterator&& iter,
      const core::fix::message_t::const_iterator& end);
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::PositionReport> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::PositionReport& value, C& ctx) {
    return format_to(
        ctx.begin(),
        "{{"
        "pos_maint_rpt_id=\"{}\", "
        "pos_req_id=\"{}\", "
        "pos_req_result={}, "
        "pos_req_type={}, "
        "..."
        "}}",
          value.pos_maint_rpt_id,
          value.pos_req_id,
          value.pos_req_result,
          value.pos_req_type);
  }
};
