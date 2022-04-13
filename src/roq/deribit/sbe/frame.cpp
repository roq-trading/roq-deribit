/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/deribit/sbe/frame.hpp"

#include "roq/exceptions.hpp"

#include "roq/core/byte_order.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {
namespace sbe {

Frame Frame::parse(const std::span<std::byte const> &buffer) {
  if (std::size(buffer) < 8)
    throw RuntimeError("Invalid message, size={}"sv, std::size(buffer));
  return {
      .packet_length = core::little_endian_to_host(
          *reinterpret_cast<decltype(Frame::packet_length) const *>(&buffer[0])),
      .channel_id = core::little_endian_to_host(
          *reinterpret_cast<decltype(Frame::channel_id) const *>(&buffer[2])),
      .sequence_number = core::little_endian_to_host(
          *reinterpret_cast<decltype(Frame::sequence_number) const *>(&buffer[4])),
  };
}

}  // namespace sbe
}  // namespace deribit
}  // namespace roq
