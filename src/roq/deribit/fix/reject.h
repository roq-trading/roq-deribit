/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include "roq/core/utils/buffer.h"
#include "roq/core/utils/message.h"

#include "roq/core/fix/reader.h"

namespace roq {
namespace deribit {
namespace fix {

struct Reject final {
  uint64_t ref_seq_num = 0;
  uint32_t ref_tag_id = 0;
  std::string_view ref_msg_type;
  std::string_view text;

  static Reject parse(const core::fix::message_t& message);
  static void parse(Reject&, const core::fix::message_t& message);

  void parse(
      core::fix::message_t::const_iterator&& iter,
      const core::fix::message_t::const_iterator& end);

  core::utils::Message encode(
      core::utils::Buffer& buffer,
      uint64_t& msg_seq_num,
      std::chrono::nanoseconds sending_time) const;
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::Reject> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::Reject& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{{"
        "ref_seq_num={}, "
        "ref_tag_id={}, "
        "ref_msg_type=\"{}\", "
        "text=\"{}\""
        "}}",
        value.ref_seq_num,
        value.ref_tag_id,
        value.ref_msg_type,
        value.text);
  }
};
