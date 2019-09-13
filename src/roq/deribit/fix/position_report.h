/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <cstddef>
#include <string_view>
#include <vector>

#include "roq/core/fix/reader.h"

#include "roq/deribit/fix/position_qty.h"

namespace roq {
namespace deribit {
namespace fix {

struct PositionReport final {
  std::string_view pos_maint_rpt_id;
  std::string_view pos_req_id;
  core::fix::PosReqResult pos_req_result = core::fix::PosReqResult::UNKNOWN;
  core::fix::PosReqType pos_req_type = core::fix::PosReqType::UNKNOWN;

  struct {
    PositionQty *items = nullptr;
    size_t length = 0;
  } positions;  // PositionQty

  static PositionReport parse(
      const core::fix::message_t& message,
      std::vector<std::byte>& buffer);

  static void parse(
      PositionReport&,
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
struct fmt::formatter<roq::deribit::fix::PositionReport> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::PositionReport& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{{"
        "pos_maint_rpt_id=\"{}\", "
        "pos_req_id=\"{}\", "
        "pos_req_result={}, "
        "pos_req_type={}, "
        "positions=[{}]"
        "}}",
        value.pos_maint_rpt_id,
        value.pos_req_id,
        value.pos_req_result,
        value.pos_req_type,
        fmt::join(
            value.positions.items,
            value.positions.items + value.positions.length,
            ", "));
  }
};
