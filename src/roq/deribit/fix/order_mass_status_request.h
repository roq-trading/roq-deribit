/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include "roq/core/utils/message.h"

#include "roq/core/fix/order_mass_status_request.h"

#include "roq/deribit/fix/deribit.h"

namespace roq {
namespace deribit {
namespace fix {

struct OrderMassStatusRequest final {
  std::string_view mass_status_req_id;
  core::fix::MassStatusReqType mass_status_req_type;

  core::utils::Message encode(
      core::utils::Buffer& buffer,
      uint64_t& msg_seq_num,
      std::chrono::nanoseconds sending_time) const;
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::OrderMassStatusRequest> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::OrderMassStatusRequest& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{{"
        "mass_status_req_id=\"{}\", "
        "mass_status_req_type={}"
        "}}",
        value.mass_status_req_id,
        value.mass_status_req_type);
  }
};
