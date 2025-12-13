/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/sbe/parser.hpp"

#include "roq/logging.hpp"

#include "roq/utils/debug/hex/message.hpp"

#include "roq/deribit/sbe/frame.hpp"
#include "roq/deribit/sbe/utils.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {
namespace sbe {

// === HELPERS ===

namespace {
auto sbe_buffer(auto &buffer) {
  return std::span{const_cast<char *>(reinterpret_cast<char const *>(std::data(buffer))), std::size(buffer)};
}

template <typename T>
auto dispatch_helper(auto &handler, auto &trace_info, auto &message, auto &frame) {
  auto tmp = sbe_buffer(message);
  T value{std::data(tmp), std::size(tmp)};
  auto bytes = compute_length(value);
  value.sbeRewind();  // note!
  create_trace_and_dispatch(handler, trace_info, value, frame);
  return bytes;
}
}  // namespace

// === IMPLEMENTATION ===

bool Parser::dispatch(Handler &handler, std::span<std::byte const> const &buffer, TraceInfo const &trace_info) {
  auto callback = [&](auto &frame, auto &packet) -> bool {
    // log::debug("frame={}"sv, frame);
    if (!handler(frame)) {
      return false;
    }
    while (!std::empty(packet)) {
      // log::debug("len(packet)={}"sv, std::size(packet));
      // log::debug("packet={}"sv, utils::debug::hex::Message{packet});
      auto length_message_header = deribit_multicast::MessageHeader::encodedLength();
      assert(std::size(packet) >= length_message_header);
      auto tmp = sbe_buffer(packet);
      deribit_multicast::MessageHeader message_header{std::data(tmp), std::size(tmp)};
      auto message = packet.subspan(length_message_header);
      auto template_id = message_header.templateId();
      // log::debug("template_id={}"sv, template_id);
      auto bytes = length_message_header;
      switch (template_id) {
        // 1000
        case deribit_multicast::Instrument::SBE_TEMPLATE_ID:
          bytes += dispatch_helper<deribit_multicast::Instrument>(handler, trace_info, message, frame);
          break;
        // 1001
        case deribit_multicast::Book::SBE_TEMPLATE_ID:
          bytes += dispatch_helper<deribit_multicast::Book>(handler, trace_info, message, frame);
          break;
        // 1002
        case deribit_multicast::Trades::SBE_TEMPLATE_ID:
          bytes += dispatch_helper<deribit_multicast::Trades>(handler, trace_info, message, frame);
          break;
        // 1003
        case deribit_multicast::Ticker::SBE_TEMPLATE_ID:
          bytes += dispatch_helper<deribit_multicast::Ticker>(handler, trace_info, message, frame);
          break;
        // 1004
        case deribit_multicast::Snapshot::SBE_TEMPLATE_ID:
          bytes += dispatch_helper<deribit_multicast::Snapshot>(handler, trace_info, message, frame);
          break;
        // 1005
        case deribit_multicast::SnapshotStart::SBE_TEMPLATE_ID:
          bytes += dispatch_helper<deribit_multicast::SnapshotStart>(handler, trace_info, message, frame);
          break;
        // 1006
        case deribit_multicast::SnapshotEnd::SBE_TEMPLATE_ID:
          bytes += dispatch_helper<deribit_multicast::SnapshotEnd>(handler, trace_info, message, frame);
          break;
        // 1007
        case deribit_multicast::ComboLegs::SBE_TEMPLATE_ID:
          bytes += dispatch_helper<deribit_multicast::ComboLegs>(handler, trace_info, message, frame);
          break;
        // 1008
        case deribit_multicast::PriceIndex::SBE_TEMPLATE_ID:
          bytes += dispatch_helper<deribit_multicast::PriceIndex>(handler, trace_info, message, frame);
          break;
        // 1009
        case deribit_multicast::Rfq::SBE_TEMPLATE_ID:
          bytes += dispatch_helper<deribit_multicast::Rfq>(handler, trace_info, message, frame);
          break;
        // 1010
        case deribit_multicast::InstrumentV2::SBE_TEMPLATE_ID:
          bytes += dispatch_helper<deribit_multicast::InstrumentV2>(handler, trace_info, message, frame);
          break;
        default:
          log::warn("payload={}"sv, utils::debug::hex::Message{buffer});
          log::fatal("Unexpected: template_id={}"sv, template_id);
      }
      // log::debug("bytes={}"sv, bytes);
      assert(bytes <= std::size(packet));
      packet = packet.subspan(bytes);
    }
    if (!std::empty(packet)) {
      log::warn("payload={}"sv, utils::debug::hex::Message{buffer});
      log::fatal("Unexpected"sv);
    }
    return true;
  };
  return Frame::parse(buffer, callback);
}

}  // namespace sbe
}  // namespace deribit
}  // namespace roq
