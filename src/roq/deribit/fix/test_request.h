/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <string_view>

#include "roq/core/utils/message.h"

#include "roq/core/fix/reader.h"
#include "roq/core/fix/test_request.h"
#include "roq/core/fix/writer.h"

namespace roq {
namespace deribit {
namespace fix {

struct TestRequest final {
  static constexpr auto MSG_TYPE = core::fix::TestRequest::msg_type;

  std::string_view test_req_id;

  static TestRequest parse(const core::fix::message_t& message);
  static void parse(TestRequest&, const core::fix::message_t& message);

  void parse(
      core::fix::message_t::const_iterator&& iter,
      const core::fix::message_t::const_iterator& end);

  core::utils::Message encode(core::fix::Writer& writer) const;
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::TestRequest> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::TestRequest& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{{"
        "test_req_id=\"{}\""
        "}}",
        value.test_req_id);
  }
};
