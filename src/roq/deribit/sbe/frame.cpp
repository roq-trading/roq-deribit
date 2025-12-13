/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/sbe/frame.hpp"

#include "roq/logging.hpp"

#include "roq/utils/byte_order.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {
namespace sbe {

// === HELPERS ===

namespace {
static_assert(sizeof(Frame) == Frame::size());
}

// === IMPLEMENTATION ===

// NOLINTBEGIN(readability-container-data-pointer)

Frame Frame::parse_helper(std::span<std::byte const> const &buffer) {
  if (std::size(buffer) < size()) {
    log::fatal("Invalid message, size={}"sv, std::size(buffer));
  }
  uint16_t packet_length;
  std::memcpy(&packet_length, &buffer[0], sizeof(packet_length));
  packet_length = utils::little_endian_to_host(packet_length);
  uint16_t channel_id;
  std::memcpy(&channel_id, &buffer[2], sizeof(channel_id));
  channel_id = utils::little_endian_to_host(channel_id);
  uint32_t sequence_number;
  std::memcpy(&sequence_number, &buffer[4], sizeof(sequence_number));
  sequence_number = utils::little_endian_to_host(sequence_number);
  return {
      .packet_length = packet_length,
      .channel_id = channel_id,
      .sequence_number = sequence_number,
  };
}

// NOLINTEND(readability-container-data-pointer)

}  // namespace sbe
}  // namespace deribit
}  // namespace roq
