/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <catch2/catch.hpp>

#include <deribit_multicast/Book.h>
#include <deribit_multicast/MessageHeader.h>
#include <deribit_multicast/Quote.h>

#include <iostream>

using namespace std::literals;

using namespace Catch::literals;

using namespace deribit_multicast;

TEST_CASE("multicast_first_test", "[multicast]") {
  auto data =
      "\x7b\x00\x03\x00\xde\x1d\x01\x00\x1d\x00\xe9\x03\x01\x00\x01\x00"  // frame + book
      "\x01\x00\x00\x00\x24\x00\x00\x00\x53\x49\x61\x19\x80\x01\x00\x00"
      "\x89\xba\x19\x00\x00\x00\x00\x00\x8a\xba\x19\x00\x00\x00\x00\x00"
      "\x01\x12\x00\x01\x00\x00\x00\x00\x00\x00\x00\xcd\xcc\xcc\xcc\x94"
      "\x8a\xe4\x40\x00\x00\x00\x00\x00\x00\x24\x40\x2c\x00\xeb\x03\x01"  // quote
      "\x00\x01\x00\x00\x00\x00\x00\x24\x00\x00\x00\x53\x49\x61\x19\x80"
      "\x01\x00\x00\xae\x47\xe1\x7a\x94\x8a\xe4\x40\x00\x00\x00\x00\x00"
      "\x00\x49\x40\xcd\xcc\xcc\xcc\x94\x8a\xe4\x40\x00\x00\x00\x00\x00"
      "\x00\x24\x40"sv;
  {
    auto message = data.substr(8);
    MessageHeader header{const_cast<char *>(std::data(message)), std::size(message)};
    CHECK(header.blockLength() == 29);
    CHECK(header.templateId() == 1001);
    CHECK(header.sbeSchemaId() == 1);
    CHECK(header.version() == 1);
    // CHECK(message_header.sbeSchemaVersion() == 1);
    std::cout << header << std::endl;
  }
  {
    auto message = data.substr(8);
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
    std::cout << book << std::endl;
  }
  {
    auto message = data.substr(75);
    Quote quote{const_cast<char *>(std::data(message)), std::size(message)};
    auto &header = quote.header();
    CHECK(header.blockLength() == 44);
    CHECK(header.templateId() == 1003);
    CHECK(header.sbeSchemaId() == 1);
    CHECK(header.version() == 1);
    //
    CHECK(quote.timestampMs() == 1649693247827);
    //
    std::cout << quote << std::endl;
  }
}
