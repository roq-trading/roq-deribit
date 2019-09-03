/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <string_view>

#include "roq/core/fix/reader.h"

namespace roq {
namespace deribit {
namespace fix {

struct TestRequest final {
  std::string_view test_req_id;

  static TestRequest parse(const core::fix::message_t& message);
  static void parse(TestRequest&, const core::fix::message_t& message);

  void parse(
      core::fix::message_t::const_iterator&& iter,
      const core::fix::message_t::const_iterator& end);
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
        ctx.begin(),
        "{{"
        "test_req_id=\"{}\""
        "}}",
        value.test_req_id);
  }
};
