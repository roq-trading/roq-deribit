/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/deribit/sbe/parser.hpp"

#include "roq/logging.hpp"

#include "roq/debug/hex/message.hpp"

#include "roq/deribit/sbe/frame.hpp"
#include "roq/deribit/sbe/utils.hpp"

#include <iostream>

using namespace std::literals;

namespace roq {
namespace deribit {
namespace sbe {

namespace {
auto dispatch_1000_instrument(auto &handler, auto &trace_info, auto &message, auto &frame) {
  deribit_multicast::Instrument instrument{std::data(message), std::size(message)};
  auto length = compute_length(instrument);
  instrument.sbeRewind();  // note! important
  create_trace_and_dispatch(handler, trace_info, instrument, frame);
  return message.subspan(length);
}

auto dispatch_1001_book(auto &handler, auto &trace_info, auto &message, auto &frame) {
  deribit_multicast::Book book{std::data(message), std::size(message)};
  auto length = compute_length(book);
  book.sbeRewind();  // note! important
  create_trace_and_dispatch(handler, trace_info, book, frame);
  return message.subspan(length);
}

auto dispatch_1002_trades(auto &handler, auto &trace_info, auto &message, auto &frame) {
  deribit_multicast::Trades trades{std::data(message), std::size(message)};
  auto length = compute_length(trades);
  trades.sbeRewind();  // note! important
  create_trace_and_dispatch(handler, trace_info, trades, frame);
  return message.subspan(length);
}

auto dispatch_1003_ticker(auto &handler, auto &trace_info, auto &message, auto &frame) {
  deribit_multicast::Ticker ticker{std::data(message), std::size(message)};
  auto length = compute_length(ticker);
  ticker.sbeRewind();  // note! important
  create_trace_and_dispatch(handler, trace_info, ticker, frame);
  return message.subspan(length);
}

auto dispatch_1004_snapshot(auto &handler, auto &trace_info, auto &message, auto &frame) {
  deribit_multicast::Snapshot snapshot{std::data(message), std::size(message)};
  auto length = compute_length(snapshot);
  snapshot.sbeRewind();  // note! important
  create_trace_and_dispatch(handler, trace_info, snapshot, frame);
  return message.subspan(length);
}

bool dispatch_helper(auto &handler, auto &trace_info, auto &message, auto &frame) {
  while (true) {
    deribit_multicast::MessageHeader header{std::data(message), std::size(message)};
    auto template_id = header.templateId();
    switch (header.templateId()) {
      case 1000:
        message = dispatch_1000_instrument(handler, trace_info, message, frame);
        break;
      case 1001:
        message = dispatch_1001_book(handler, trace_info, message, frame);
        break;
      case 1002:
        message = dispatch_1002_trades(handler, trace_info, message, frame);
        break;
      case 1003:
        message = dispatch_1003_ticker(handler, trace_info, message, frame);
        break;
      case 1004:
        message = dispatch_1004_snapshot(handler, trace_info, message, frame);
        break;
      default: {
        log::warn("Unexpected: template_id={}"sv, template_id);
        return true;
      }
    }
    if (std::empty(message))
      return false;
    // XXX something wrong with Snapshot...
    if (std::size(message) < 12) {  // size of header
      log::warn("remaining data: length={}"sv, std::size(message));
      return true;
    }
  }
}
}  // namespace

size_t Parser::dispatch(Handler &handler, std::span<std::byte const> const &buffer, TraceInfo const &trace_info) {
  size_t total_bytes = 0;
  auto failed = false;
  while (!failed) {
    auto tmp = buffer.subspan(total_bytes);
    if (std::empty(tmp))
      break;
    if (Frame::parse(tmp, [&](auto &frame) {
          auto length = std::size(frame) + frame.packet_length;
          if (std::size(tmp) < (std::size(frame) + frame.packet_length)) {
            log::warn("+++ INCOMPLETE DATAGRAM +++"sv);
            failed = true;
          } else {
            total_bytes += length;
            auto tmp_2 = tmp.subspan(Frame::size(), frame.packet_length);
            // note! sbe headers are not const-safe
            std::span message{reinterpret_cast<char *>(const_cast<std::byte *>(std::data(tmp_2))), std::size(tmp_2)};
            failed = dispatch_helper(handler, trace_info, message, frame);
          }
        })) {
    } else {
      break;  // unable to decode a frame header
    }
  }
  if (failed) [[unlikely]]
    log::fatal("{}"sv, debug::hex::Message{buffer});
  return total_bytes;
}

}  // namespace sbe
}  // namespace deribit
}  // namespace roq
