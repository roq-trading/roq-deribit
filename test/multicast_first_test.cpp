/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <catch2/catch.hpp>

#include <deribit_multicast/Book.h>
#include <deribit_multicast/MessageHeader.h>
#include <deribit_multicast/Quote.h>

#include <iostream>

#include "roq/deribit/sbe/frame.hpp"
#include "roq/deribit/sbe/utils.hpp"

using namespace std::literals;

using namespace Catch::literals;

using namespace deribit_multicast;

using namespace roq::deribit::sbe;

TEST_CASE("multicast_first_test", "[multicast]") {
  //  0-  7 |  8 : frame
  //  8- 74 | 67 : book
  // 75-130 | 56 : quote
  // ---
  // frame = 8
  // header = 12
  // book = 56 = 29 + 2 * 18
  // quote = 44 = 44
  auto buffer =
      "\x7b\x00\x03\x00\xde\x1d\x01\x00\x1d\x00\xe9\x03\x01\x00\x01\x00"  // frame + book
      "\x01\x00\x00\x00\x24\x00\x00\x00\x53\x49\x61\x19\x80\x01\x00\x00"
      "\x89\xba\x19\x00\x00\x00\x00\x00\x8a\xba\x19\x00\x00\x00\x00\x00"
      "\x01\x12\x00\x01\x00\x00\x00\x00\x00\x00\x00\xcd\xcc\xcc\xcc\x94"
      "\x8a\xe4\x40\x00\x00\x00\x00\x00\x00\x24\x40\x2c\x00\xeb\x03\x01"  // quote
      "\x00\x01\x00\x00\x00\x00\x00\x24\x00\x00\x00\x53\x49\x61\x19\x80"
      "\x01\x00\x00\xae\x47\xe1\x7a\x94\x8a\xe4\x40\x00\x00\x00\x00\x00"
      "\x00\x49\x40\xcd\xcc\xcc\xcc\x94\x8a\xe4\x40\x00\x00\x00\x00\x00"
      "\x00\x24\x40"sv;
  CHECK(std::size(buffer) == 131);
  {
    auto frame =
        Frame::parse({reinterpret_cast<std::byte const *>(std::data(buffer)), std::size(buffer)});
    CHECK(frame.packet_length == 123);
    CHECK(frame.channel_id == 3);
    CHECK(frame.sequence_number == 73182);
  }
  {
    auto message = buffer.substr(8);
    MessageHeader header{const_cast<char *>(std::data(message)), std::size(message)};
    CHECK(header.blockLength() == 29);
    CHECK(header.templateId() == 1001);  // defines the parser (1001=book)
    CHECK(header.sbeSchemaId() == 1);
    CHECK(header.version() == 1);
  }
  {
    auto message = buffer.substr(8);
    Book book{const_cast<char *>(std::data(message)), std::size(message)};

    auto &header = book.header();
    CHECK(header.blockLength() == 29);
    CHECK(header.templateId() == 1001);
    CHECK(header.sbeSchemaId() == 1);
    CHECK(header.version() == 1);
    //
    CHECK(book.timestampMs() == 1649693247827);
    CHECK(book.prevChangeId() == 1686153);
    CHECK(book.changeId() == 1686154);
    CHECK(book.isLast() == true);
    //
    CHECK(book.computeLength() == 49);
    CHECK(book.changesList().computeLength() == 18);
    CHECK((49 + 18) == 67);
    //
    // fmt::print("{}\n"sv, book);
  }
  {
    auto message = buffer.substr(75);
    Quote quote{const_cast<char *>(std::data(message)), std::size(message)};
    auto &header = quote.header();
    CHECK(header.blockLength() == 44);
    CHECK(header.templateId() == 1003);
    CHECK(header.sbeSchemaId() == 1);
    CHECK(header.version() == 1);
    //
    CHECK(quote.timestampMs() == 1649693247827);
    //
    // fmt::print("{}\n"sv, quote);
    //
    CHECK(quote.computeLength() == 56);
  }
}
