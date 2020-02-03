/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include "roq/core/utils/message.h"

#include "roq/core/fix/heartbeat.h"
#include "roq/core/fix/reader.h"
#include "roq/core/fix/writer.h"

#include "roq/deribit/fix/deribit.h"

namespace roq {
namespace deribit {
namespace fix {

struct Heartbeat final {
  std::string_view test_req_id;

 public:
  static constexpr auto msg_type = core::fix::Heartbeat::msg_type;

  Heartbeat() = default;
  Heartbeat(Heartbeat&&) = default;
  Heartbeat(const Heartbeat&) = delete;

  static Heartbeat create(const core::fix::message_t& message);

  core::utils::Message encode(core::fix::Writer& writer) const;
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::fix::Heartbeat> {
  template <typename C>
  constexpr auto parse(C& ctx) {
    return ctx.begin();
  }
  template <typename C>
  auto format(const roq::deribit::fix::Heartbeat& value, C& ctx) {
    return format_to(
        ctx.out(),
        "{{"
        "test_req_id=\"{}\""
        "}}",
        value.test_req_id);
  }
};
