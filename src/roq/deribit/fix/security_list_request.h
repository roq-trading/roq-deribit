/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include "roq/core/utils/message.h"

#include "roq/core/fix/security_list_request.h"
#include "roq/core/fix/writer.h"

#include "roq/deribit/fix/deribit.h"

namespace roq {
namespace deribit {
namespace fix {

struct SecurityListRequest final {
  static constexpr auto MSG_TYPE = core::fix::SecurityListRequest::msg_type;

  std::string_view security_req_id;
  core::fix::SecurityListRequestType security_list_request_type =
    core::fix::SecurityListRequestType::ALL_SECURITIES;

  core::utils::Message encode(core::fix::Writer& writer) const;
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::SecurityListRequest> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::SecurityListRequest& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{{"
        "security_req_id=\"{}\", "
        "security_list_request_type={}"
        "}}",
        value.security_req_id,
        value.security_list_request_type);
  }
};
