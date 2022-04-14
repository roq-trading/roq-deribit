/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <cstdint>
#include <span>

namespace roq {
namespace deribit {
namespace sbe {

struct Frame final {
  uint16_t packet_length = {};
  uint16_t channel_id = {};
  uint32_t sequence_number = {};

  static size_t size() { return 8; }

  template <typename Callback>
  static bool parse(const std::span<std::byte const> &buffer, Callback &&callback) {
    auto [result, frame] = parse_helper(buffer);
    if (result)
      callback(frame);
    return result;
  }

 private:
  static std::pair<bool, Frame> parse_helper(const std::span<std::byte const> &buffer);
};

}  // namespace sbe
}  // namespace deribit
}  // namespace roq
