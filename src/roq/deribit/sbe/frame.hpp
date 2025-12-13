/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <cstdint>
#include <span>

namespace roq {
namespace deribit {
namespace sbe {

struct Frame final {
  uint16_t packet_length = {};
  uint16_t channel_id = {};
  uint32_t sequence_number = {};

  static constexpr size_t size() { return 8; }

  template <typename Callback>
  static bool parse(std::span<std::byte const> const &buffer, Callback callback) {
    auto frame = parse_helper(buffer);
    auto packet = buffer.subspan(sizeof(Frame));
    return callback(frame, packet);
  }

 private:
  static Frame parse_helper(std::span<std::byte const> const &buffer);
};

}  // namespace sbe
}  // namespace deribit
}  // namespace roq

template <>
struct fmt::formatter<roq::deribit::sbe::Frame> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::deribit::sbe::Frame const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(packet_length={}, )"
        R"(channel_id={}, )"
        R"(sequence_number={})"
        R"(}})"sv,
        value.packet_length,
        value.channel_id,
        value.sequence_number);
  }
};
