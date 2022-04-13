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

  static Frame parse(const std::span<std::byte const> &buffer);

  static size_t size() { return 8; }
};

}  // namespace sbe
}  // namespace deribit
}  // namespace roq
