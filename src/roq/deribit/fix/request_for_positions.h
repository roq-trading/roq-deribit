/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include "roq/core/utils/message.h"

#include "roq/core/fix/request_for_positions.h"
#include "roq/core/fix/writer.h"

#include "roq/deribit/fix/deribit.h"

namespace roq {
namespace deribit {
namespace fix {

struct RequestForPositions final {
  static constexpr auto MSG_TYPE = core::fix::RequestForPositions::msg_type;

  std::string_view pos_req_id;
  core::fix::PosReqType pos_req_type = core::fix::PosReqType::UNKNOWN;

  core::utils::Message encode(core::fix::Writer& writer) const;
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::RequestForPositions> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::RequestForPositions& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{{"
        "pos_req_id=\"{}\", "
        "pos_req_type={}"
        "}}",
        value.pos_req_id,
        value.pos_req_type);
  }
};
