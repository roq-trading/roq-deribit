/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/deribit/sbe/parser.hpp"

#include "roq/logging.hpp"

#include "roq/deribit/sbe/frame.hpp"
#include "roq/deribit/sbe/utils.hpp"

#include <iostream>

using namespace std::literals;

namespace roq {
namespace deribit {
namespace sbe {

bool Parser::dispatch(Handler &handler, const std::span<std::byte const> &buffer) {
  auto result = true;
  if (Frame::parse(buffer, [&](auto &frame) {
        auto tmp = buffer.subspan(Frame::size());
        // sbe headers are not const-safe
        std::span message{
            reinterpret_cast<char *>(const_cast<std::byte *>(std::data(tmp))), std::size(tmp)};
        while (result) {
          deribit_multicast::MessageHeader header{std::data(message), std::size(message)};
          auto template_id = header.templateId();
          switch (header.templateId()) {
            case 1000: {
              deribit_multicast::Instrument instrument{std::data(message), std::size(message)};
              auto length = compute_length(instrument);
              // log::info<5>("instrument={}"sv, instrument);
              instrument.sbeRewind();  // note! important
              handler(frame.channel_id, frame.sequence_number, instrument);
              message = message.subspan(length);
              break;
            }
            case 1001: {
              deribit_multicast::Book book{std::data(message), std::size(message)};
              auto length = compute_length(book);
              // log::info<5>("book={}"sv, book);
              book.sbeRewind();  // note! important
              handler(frame.channel_id, frame.sequence_number, book);
              message = message.subspan(length);
              break;
            }
            case 1002: {
              deribit_multicast::Trades trades{std::data(message), std::size(message)};
              auto length = compute_length(trades);
              // log::info<5>("trades={}"sv, trades);
              trades.sbeRewind();  // note! important
              handler(frame.channel_id, frame.sequence_number, trades);
              message = message.subspan(length);
              break;
            }
            case 1003: {
              deribit_multicast::Quote quote{std::data(message), std::size(message)};
              auto length = compute_length(quote);
              // log::info<5>("quote={}"sv, quote);
              quote.sbeRewind();  // note! important
              handler(frame.channel_id, frame.sequence_number, quote);
              message = message.subspan(length);
              break;
            }
            case 1004: {
              deribit_multicast::Snapshot snapshot{std::data(message), std::size(message)};
              auto length = compute_length(snapshot);
              // std::cerr << snapshot << std::endl;
              // log::info<5>("snapshot={}"sv, snapshot);
              snapshot.sbeRewind();  // note! important
              handler(frame.channel_id, frame.sequence_number, snapshot);
              message = message.subspan(length);
              break;
            }
            default: {
              log::warn("Unknown template_id={}"sv, template_id);
              result = false;
              return;
            }
          }
          if (std::empty(message))
            break;
          // XXX something wrong with Snapshot...
          if (std::size(message) < 12) {  // size of header
            log::warn("remaining data: length={}"sv, std::size(message));
            break;
          }
        }
      })) {
  } else {
    return false;
  }
  return result;
}

}  // namespace sbe
}  // namespace deribit
}  // namespace roq
