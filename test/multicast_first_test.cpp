/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <catch2/catch.hpp>

#include <deribit_multicast/Book.h>
#include <deribit_multicast/MessageHeader.h>
#include <deribit_multicast/Quote.h>

#include <iostream>

#include "roq/deribit/sbe/frame.hpp"
#include "roq/deribit/sbe/parser.hpp"
#include "roq/deribit/sbe/utils.hpp"

#include "roq/logging.hpp"

using namespace std::literals;

using namespace Catch::literals;

using namespace deribit_multicast;

using namespace roq;
using namespace roq::deribit;

TEST_CASE("multicast_example", "[multicast]") {
  //  0-  7 |  8 : frame
  //  8- 74 | 67 : book
  // 75-130 | 56 : quote
  // ---
  // frame = 8
  // header = 12
  // book = 56 = 29 + 2 * 18
  // quote = 44 = 44
  auto buffer =
      "\x7b\x00\x03\x00\xde\x1d\x01\x00\x1d\x00\xe9\x03\x01\x00\x01\x00"  // [0] frame + [8] book
      "\x01\x00\x00\x00\x24\x00\x00\x00\x53\x49\x61\x19\x80\x01\x00\x00"
      "\x89\xba\x19\x00\x00\x00\x00\x00\x8a\xba\x19\x00\x00\x00\x00\x00"
      "\x01\x12\x00\x01\x00\x00\x00\x00\x00\x00\x00\xcd\xcc\xcc\xcc\x94"
      "\x8a\xe4\x40\x00\x00\x00\x00\x00\x00\x24\x40\x2c\x00\xeb\x03\x01"  // [75] quote
      "\x00\x01\x00\x00\x00\x00\x00\x24\x00\x00\x00\x53\x49\x61\x19\x80"
      "\x01\x00\x00\xae\x47\xe1\x7a\x94\x8a\xe4\x40\x00\x00\x00\x00\x00"
      "\x00\x49\x40\xcd\xcc\xcc\xcc\x94\x8a\xe4\x40\x00\x00\x00\x00\x00"
      "\x00\x24\x40"sv;
  CHECK(std::size(buffer) == 131);
  {
    auto result = sbe::Frame::parse(
        {reinterpret_cast<std::byte const *>(std::data(buffer)), std::size(buffer)},
        [](auto &frame) {
          CHECK(frame.packet_length == 123);
          CHECK(frame.channel_id == 3);
          CHECK(frame.sequence_number == 73182);
        });
    CHECK(result == true);
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
    CHECK(sbe::compute_length(book) == 67);
    CHECK(book.computeLength() == 49);
    CHECK(book.changesList().computeLength() == 18);
    CHECK((49 + 18) == 67);
    //
    /*
    book.sbeRewind();
    {
      auto &xxx = book.changesList();
      if (xxx.hasNext()) {
        roq::core::sbe::iterator iter{xxx};
        fmt::print("{}\n"sv, *iter);
      }
    }
    */
    fmt::print("{}\n"sv, book);
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
    fmt::print("{}\n"sv, quote);
    //
    CHECK(quote.computeLength() == 56);
  }
}

TEST_CASE("multicast_1001_book", "[multicast]") {
  auto message =
      "\x55\x00\x0a\x00\xb0\x4a\x3b\x04\x1d\x00\xe9\x03\x01\x00\x01\x00"
      "\x01\x00\x00\x00\xf8\x3a\x03\x00\x61\xcb\x5c\x27\x80\x01\x00\x00"
      "\x32\x60\x07\x14\x00\x00\x00\x00\x33\x60\x07\x14\x00\x00\x00\x00"
      "\x01\x12\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
      "\x18\xe4\x40\x5a\x64\x3b\xdf\x4f\x8d\xa7\x3f\x00\x02\x00\x00\x00"
      "\x00\x60\x18\xe4\x40\x00\x00\x00\x00\x00\x00\x00\x00"sv;
  CHECK(std::size(message) == 93);
  CHECK(message[0] == 0x55);
  CHECK(message[1] == 0x00);
  CHECK(message[2] == 0x0a);
  CHECK(message[3] == 0x00);
  const std::span buffer{
      reinterpret_cast<std::byte const *>(std::data(message)), std::size(message)};
  CHECK(std::size(buffer) == 93);
  CHECK(buffer[0] == std::byte{0x55});
  CHECK(buffer[1] == std::byte{0x00});
  CHECK(buffer[2] == std::byte{0x0a});
  CHECK(buffer[3] == std::byte{0x00});
  {
    auto result = sbe::Frame::parse(buffer, [](auto &frame) {
      CHECK(frame.packet_length == 85);
      CHECK(frame.channel_id == 10);
      CHECK(frame.sequence_number == 70994608);
    });
    CHECK(result == true);
  }
  {
    auto message = buffer.subspan(8);
    MessageHeader header{
        reinterpret_cast<char *>(const_cast<std::byte *>(std::data(message))), std::size(message)};
    CHECK(header.blockLength() == 29);
    CHECK(header.templateId() == 1001);  // defines the parser (1001=book)
    CHECK(header.sbeSchemaId() == 1);
    CHECK(header.version() == 1);
  }
}

TEST_CASE("multicast_book", "[multicast]") {
  auto message =
      "\x55\x00\x0a\x00\xb0\x4a\x3b\x04\x1d\x00\xe9\x03\x01\x00\x01\x00"
      "\x01\x00\x00\x00\xf8\x3a\x03\x00\x61\xcb\x5c\x27\x80\x01\x00\x00"
      "\x32\x60\x07\x14\x00\x00\x00\x00\x33\x60\x07\x14\x00\x00\x00\x00"
      "\x01\x12\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
      "\x18\xe4\x40\x5a\x64\x3b\xdf\x4f\x8d\xa7\x3f\x00\x02\x00\x00\x00"
      "\x00\x60\x18\xe4\x40\x00\x00\x00\x00\x00\x00\x00\x00"sv;
  std::span buffer{reinterpret_cast<std::byte const *>(std::data(message)), std::size(message)};
  struct MyHandler : public sbe::Parser::Handler {
    bool found = false;
    void operator()(const Trace<deribit_multicast::Instrument> &, const sbe::Frame &) override {
      FAIL();
    }
    void operator()(const Trace<deribit_multicast::Book> &event, const sbe::Frame &frame) override {
      found = true;
      CHECK(frame.channel_id == 10);
      CHECK(frame.sequence_number == 70994608);
      auto &[trace_info, book] = event;
      auto &header = book.header();
      CHECK(header.sbeSchemaId() == 1);
      CHECK(header.version() == 1);
    }
    void operator()(const Trace<deribit_multicast::Quote> &, const sbe::Frame &) override {
      FAIL();
    }
    void operator()(const Trace<deribit_multicast::Trades> &, const sbe::Frame &) override {
      FAIL();
    }
    // snapshot
    void operator()(const Trace<deribit_multicast::Snapshot> &, const sbe::Frame &) override {
      FAIL();
    }
  } handler;
  TraceInfo trace_info;
  auto result = sbe::Parser::dispatch(handler, buffer, trace_info);
  CHECK(result == true);
  CHECK(handler.found == true);
}

TEST_CASE("multicast_book_quote", "[multicast]") {
  auto message =
      "\x8d\x00\x0a\x00\xfd\xe3\x3d\x04\x1d\x00\xe9\x03\x01\x00\x01\x00"  // [0] frame + [8] book
      "\x01\x00\x00\x00\x2f\x40\x03\x00\x01\xa1\x6f\x27\x80\x01\x00\x00"
      "\x31\xa3\x0a\x14\x00\x00\x00\x00\x32\xa3\x0a\x14\x00\x00\x00\x00"
      "\x01\x12\x00\x02\x00\x00\x00\x00\x00\x01\x01\x33\x33\x33\x33\x33"
      "\x63\x5a\x40\x33\x33\x33\x33\x33\xb3\x42\x40\x01\x01\xa4\x70\x3d"
      "\x0a\xd7\x63\x5a\x40\x9a\x99\x99\x99\x99\x19\x44\x40\x2c\x00\xeb"  // [93] quote
      "\x03\x01\x00\x01\x00\x00\x00\x00\x00\x2f\x40\x03\x00\x01\xa1\x6f"
      "\x27\x80\x01\x00\x00\xa4\x70\x3d\x0a\xd7\x63\x5a\x40\x9a\x99\x99"
      "\x99\x99\x19\x44\x40\xb8\x1e\x85\xeb\x51\x68\x5a\x40\xcd\xcc\xcc"
      "\xcc\xcc\xcc\x30\x40"sv;
  std::span buffer{reinterpret_cast<std::byte const *>(std::data(message)), std::size(message)};
  {
    auto message = buffer.subspan(8);
    Book book{
        reinterpret_cast<char *>(const_cast<std::byte *>(std::data(message))), std::size(message)};
    CHECK(sbe::compute_length(book) == 85);
  }
  struct MyHandler : public sbe::Parser::Handler {
    bool found_book = false;
    bool found_quote = false;
    void operator()(const Trace<deribit_multicast::Instrument> &, const sbe::Frame &) override {
      FAIL();
    }
    void operator()(const Trace<deribit_multicast::Book> &event, const sbe::Frame &frame) override {
      found_book = true;
      CHECK(frame.channel_id == 10);
      CHECK(frame.sequence_number == 71164925);
      auto &[trace_info, book] = event;
      auto &header = book.header();
      CHECK(header.sbeSchemaId() == 1);
      CHECK(header.version() == 1);
    }
    void operator()(
        const Trace<deribit_multicast::Quote> &event, const sbe::Frame &frame) override {
      found_quote = true;
      CHECK(frame.channel_id == 10);
      CHECK(frame.sequence_number == 71164925);
      auto &[trace_info, quote] = event;
      auto &header = quote.header();
      CHECK(header.sbeSchemaId() == 1);
      CHECK(header.version() == 1);
    }
    void operator()(const Trace<deribit_multicast::Trades> &, const sbe::Frame &) override {
      FAIL();
    }
    // snapshot
    void operator()(const Trace<deribit_multicast::Snapshot> &, const sbe::Frame &) override {
      FAIL();
    }
  } handler;
  TraceInfo trace_info;
  auto result = sbe::Parser::dispatch(handler, buffer, trace_info);
  CHECK(result == true);
  CHECK(handler.found_book == true);
  CHECK(handler.found_quote == true);
}

TEST_CASE("multicast_snapshot", "[multicast]") {
  auto message =
      "\x39\x04\x6e\x00\xf5\x06\x02\x00\x16\x00\xec\x03\x01\x00\x01\x00"
      "\x01\x00\x01\x00\x2f\x40\x03\x00\xc4\xf4\x6f\x27\x80\x01\x00\x00"
      "\x14\xb1\x0a\x14\x00\x00\x00\x00\x00\x01\x11\x00\x3c\x00\x00\x00"
      "\x00\x00\x00\x0a\xd7\xa3\x70\x3d\x6a\x5a\x40\xcd\xcc\xcc\xcc\xcc"
      "\xcc\x2e\x40\x01\x85\xeb\x51\xb8\x1e\x65\x5a\x40\x00\x00\x00\x00"
      "\x00\x60\x53\x40\x00\x7b\x14\xae\x47\xe1\x6a\x5a\x40\x66\x66\x66"
      "\x66\x66\x46\x50\x40\x01\x33\x33\x33\x33\x33\x63\x5a\x40\x66\x66"
      "\x66\x66\x66\x66\x42\x40\x00\xec\x51\xb8\x1e\x85\x6b\x5a\x40\x33"
      "\x33\x33\x33\x33\x33\x13\x40\x01\x52\xb8\x1e\x85\xeb\x61\x5a\x40"
      "\x33\x33\x33\x33\x33\x83\x7f\x40\x00\x5c\x8f\xc2\xf5\x28\x6c\x5a"
      "\x40\x00\x00\x00\x00\x00\x00\x49\x40\x01\xe1\x7a\x14\xae\x47\x61"
      "\x5a\x40\x9a\x99\x99\x99\x99\x19\x33\x40\x00\x3d\x0a\xd7\xa3\x70"
      "\x6d\x5a\x40\x9a\x99\x99\x99\x99\x5d\x83\x40\x01\x71\x3d\x0a\xd7"
      "\xa3\x60\x5a\x40\x66\x66\x66\x66\x66\x26\x50\x40\x00\xae\x47\xe1"
      "\x7a\x14\x6e\x5a\x40\x66\x66\x66\x66\x66\x66\x3c\x40\x01\x00\x00"
      "\x00\x00\x00\x60\x5a\x40\x00\x00\x00\x00\x00\x00\x31\x40\x00\x1f"
      "\x85\xeb\x51\xb8\x6e\x5a\x40\x33\x33\x33\x33\x33\xb3\x47\x40\x01"
      "\xae\x47\xe1\x7a\x14\x5e\x5a\x40\x00\x00\x00\x00\x00\x30\x62\x40"
      "\x00\x8f\xc2\xf5\x28\x5c\x6f\x5a\x40\x66\x66\x66\x66\x66\x66\x3c"
      "\x40\x01\xcd\xcc\xcc\xcc\xcc\x5c\x5a\x40\x9a\x99\x99\x99\x99\xa9\x61\x40\x00\xc3\xf5\x28\x5c\x8f\x72\x5a\x40\x9a\x99\x99\x99\x99\x79\x6a\x40\x01\xec\x51\xb8\x1e\x85\x5b\x5a\x40\x00\x00\x00\x00\x00\x00\x49\x40\x00\x33\x33\x33\x33\x33\x73\x5a\x40\x00\x00\x00\x00\x00\xc0\x61\x40\x01\x7b\x14\xae\x47\xe1\x5a\x5a\x40\x00\x00\x00\x00\x00\x00\x41\x40\x00\xa4\x70\x3d\x0a\xd7\x73\x5a\x40\x00\x00\x00\x00\x00\x40\x7f\x40\x01\x9a\x99\x99\x99\x99\x59\x5a\x40\xcd\xcc\xcc\xcc\xcc\xac\x51\x40\x00\x85\xeb\x51\xb8\x1e\x75\x5a\x40\x9a\x99\x99\x99\x99\x99\x3b\x40\x01\xb8\x1e\x85\xeb\x51\x58\x5a\x40\x00\x00\x00\x00\x00\x40\x7f\x40\x00\xf6\x28\x5c\x8f\xc2\x75\x5a\x40\x00\x00\x00\x00\x00\x00\xf0\x3f\x01\x48\xe1\x7a\x14\xae\x57\x5a\x40\x9a\x99\x99\x99\x99\xb9\x53\x40\x00\x48\xe1\x7a\x14\xae\x77\x5a\x40\x00\x00\x00\x00\x00\xc0\x71\x40\x01\xf6\x28\x5c\x8f\xc2\x55\x5a\x40\x00\x00\x00\x00\x00\x78\x83\x40\x00\x9a\x99\x99\x99\x99\x79\x5a\x40\x00\x00\x00\x00\x00\x78\x83\x40\x01\x85\xeb\x51\xb8\x1e\x55\x5a\x40\x00\x00\x00\x00\x00\x40\x55\x40\x00\x7b\x14\xae\x47\xe1\x7a\x5a\x40\xcd\xcc\xcc\xcc\xcc\x8c\x5d\x40\x01\x33\x33\x33\x33\x33\x53\x5a\x40\xcd\xcc\xcc\xcc\xcc\x8c\x5d\x40\x00\x33\x33\x33\x33\x33\x83\x5a\x40\xcd\xcc\xcc\xcc\xcc\x36\x9d\x40\x01\x52\xb8\x1e\x85\xeb\x51\x5a\x40\x00\x00\x00\x00\x00\xe0\x5f\x40\x00\xb8\x1e\x85\xeb\x51\x88\x5a\x40\x00\x00\x00\x00\x00\x40\x8b\x40\x01\x8f\xc2\xf5\x28\x5c\x4f\x5a\x40\x00\x00\x00\x00\x00\x40\x8b\x40\x00\x8f\xc2\xf5\x28\x5c\x8f\x5a\x40\x33\x33\x33\x33\x33\x43\x78\x40\x01\xec\x51\xb8\x1e\x85\x4b\x5a\x40\x66\x66\x66\x66\x66\x66\x37\x40\x00\x29\x5c\x8f\xc2\xf5\xa8\x5a\x40\x00\x00\x00\x00\x00\x00\xf0\x3f\x01\x7b\x14\xae\x47\xe1\x4a\x5a\x40\x00\x00\x00\x00\x00\x40\x65\x40\x00\xd7\xa3\x70\x3d\x0a\xb7\x5a\x40\x00\x00\x00\x00\x00\x00\xf0\x3f\x01\x0a\xd7\xa3\x70\x3d\x4a\x5a\x40\x33\x33\x33\x33\x33\x03\x7e\x40\x00\x3d\x0a\xd7\xa3\x70\xbd\x5a\x40\x00\x00\x00\x00\x00\x60\x79\x40\x01\x48\xe1\x7a\x14\xae\x47\x5a\x40\x00\x00\x00\x00\x00\x90\x6a\x40\x00\x29\x5c\x8f\xc2\xf5\xd8\x5a\x40\x33\x33\x33\x33\x33\x93\x78\x40\x01\xd7\xa3\x70\x3d\x0a\x47\x5a\x40\xcd\xcc\xcc\xcc\xcc\x36\x9d\x40\x00\x5c\x8f\xc2\xf5\x28\xdc\x5a\x40\x00\x00\x00\x00\x00\x00\xf0\x3f\x01\xe1\x7a\x14\xae\x47\x41\x5a\x40\x33\x33\x33\x33\x33\xb3\x78\x40\x00\x00\x00\x00\x00\x00\x00\x5b\x40\xcd\xcc\xcc\xcc\x4c\x88\xb3\x40\x01\xb8\x1e\x85\xeb\x51\x28\x5a\x40\x66\x66\x66\x66\x66\x66\x37\x40\x00\x8f\xc2\xf5\x28\x5c\x0f\x5b\x40\x00\x00\x00\x00\x00\x00\xf0\x3f\x01\x48\xe1\x7a\x14\xae\x27\x5a\x40\x00\x00\x00\x00\x00\xf0\x77\x40\x00\xb8\x1e\x85\xeb\x51\x18\x5b\x40\x9a\x99\x99\x99\x99\x39\x54\x40\x01\xec\x51\xb8\x1e\x85\x1b\x5a\x40\x00\x00\x00\x00\x00\x00\xf0\x3f\x00\x00\x00\x00\x00\x00\x40\x5b\x40\x33\x33\x33\x33\x33\x33\xd3\x3f\x01\xcd\xcc\xcc\xcc\xcc\x0c\x5a\x40\x00\x00\x00\x00\x00\x00\xf0\x3f\x00\xc3\xf5\x28\x5c\x8f\x42\x5b\x40\x00\x00\x00\x00\x00\x00\xf0\x3f\x01\x00\x00\x00\x00\x00\x00\x5a\x40\x33\x33\x33\x33\x33\x33\xd3\x3f\x00\x33\x33\x33\x33\x33\x43\x5b\x40\x00\x00\x00\x00\x00\x00\x44\x40\x01\xb8\x1e\x85\xeb\x51\xf8\x59\x40\xcd\xcc\xcc\xcc\xcc\xdc\x6d\x40\x12\x53\x4f\x4c\x5f\x55\x53\x44\x43\x2d\x50\x45\x52\x50\x45\x54\x55\x41\x4c"sv;
  std::span buffer{reinterpret_cast<std::byte const *>(std::data(message)), std::size(message)};
  struct MyHandler : public sbe::Parser::Handler {
    bool found = false;
    void operator()(const Trace<deribit_multicast::Instrument> &, const sbe::Frame &) override {
      FAIL();
    }
    void operator()(const Trace<deribit_multicast::Book> &, const sbe::Frame &) override { FAIL(); }
    void operator()(const Trace<deribit_multicast::Quote> &, const sbe::Frame &) override {
      FAIL();
    }
    void operator()(const Trace<deribit_multicast::Trades> &, const sbe::Frame &) override {
      FAIL();
    }
    // snapshot
    void operator()(const Trace<deribit_multicast::Snapshot> &event, const sbe::Frame &) override {
      found = true;
      auto &[trace_info, snapshot] = event;
      snapshot.sbeRewind();
      CHECK(snapshot.instrumentId() == 213039);
      CHECK(snapshot.timestampMs() == 1649929090244);
      CHECK(snapshot.changeId() == 336245012);
      CHECK(snapshot.isBookComplete() == deribit_multicast::YesNo::no);
      CHECK(snapshot.isLastInBook() == deribit_multicast::YesNo::yes);
      size_t count = 0;
      snapshot.levelsList().forEach([&count](auto &) { ++count; });
      CHECK(count == 60);
      auto length = snapshot.instrumentNameLength();  // must fetch before getting name
      CHECK(length == 18);
      auto name = std::string_view{snapshot.instrumentName(), length};
      CHECK(name == "SOL_USDC-PERPETUAL"sv);
      fmt::print("{}\n"sv, snapshot);
    }
  } handler;
  TraceInfo trace_info;
  auto result = sbe::Parser::dispatch(handler, buffer, trace_info);
  CHECK(result == true);
  CHECK(handler.found == true);
}

TEST_CASE("multicast_instrument", "[multicast]") {
  auto message =
      "\x2a\x00\x02\x00\x00\x00\x00\x00\x05\x00\xe8\x03\x01\x00\x01\x00"
      "\x00\x00\x01\x00\x05\x00\x00\x00\x00\x18\x42\x54\x43\x2d\x31\x34"
      "\x41\x50\x52\x32\x32\x5f\x31\x35\x30\x30\x2d\x33\x39\x39\x30\x30"
      "\x2d\x43"sv;
  std::span buffer{reinterpret_cast<std::byte const *>(std::data(message)), std::size(message)};
  struct MyHandler : public sbe::Parser::Handler {
    bool found = false;
    void operator()(
        const Trace<deribit_multicast::Instrument> &event, const sbe::Frame &) override {
      found = true;
      auto &[trace_info, instrument] = event;
      CHECK(instrument.instrumentId() == 5);
      CHECK(instrument.state() == deribit_multicast::InstrumentState::Value::created);
      // instrument.sbeRewind(); // important
      auto length = instrument.instrumentNameLength();  // must fetch before getting name
      auto name = std::string_view{instrument.instrumentName(), length};
      CHECK(name == "BTC-14APR22_1500-39900-C"sv);
    }
    void operator()(const Trace<deribit_multicast::Book> &, const sbe::Frame &) override { FAIL(); }
    void operator()(const Trace<deribit_multicast::Quote> &, const sbe::Frame &) override {
      FAIL();
    }
    void operator()(const Trace<deribit_multicast::Trades> &, const sbe::Frame &) override {
      FAIL();
    }
    // snapshot
    void operator()(const Trace<deribit_multicast::Snapshot> &, const sbe::Frame &) override {
      FAIL();
    }
  } handler;
  TraceInfo trace_info;
  auto result = sbe::Parser::dispatch(handler, buffer, trace_info);
  CHECK(result == true);
  CHECK(handler.found == true);
}
