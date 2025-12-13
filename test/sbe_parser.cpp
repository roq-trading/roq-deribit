/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/deribit/sbe/frame.hpp"
#include "roq/deribit/sbe/parser.hpp"
#include "roq/deribit/sbe/utils.hpp"

using namespace std::literals;

using namespace Catch::literals;

using namespace deribit_multicast;

using namespace roq;
using namespace roq::deribit;

TEST_CASE("sbe_event_1", "[sbe_parser]") {
  auto message =
      // --- frame
      "\x3f\x01"          // package length
      "\x01\x00"          // channel (1)
      "\x6e\x0e\x06\x03"  // sequence number
      // --- message header
      "\x04\x00"  // block length (4)
      "\xea\x03"  // template id (1002)
      "\x01\x00"  // schema id (1)
      "\x02\x00"  // version (2)
      "\x01\x00"  // num groups (1)
      "\x00\x00"  // num var data fields (0)
      // --- trades
      "\x96\x37\x03\x00"  // instrument id
      "\x53\x00"          // block length (83)
      "\x01\x00"          // num in group (1)
      "\x00\x00"          // num groups
      "\x00\x00"          // num var data fields
      // --- trades[0]
      "\x01"                              // direction
      "\x00\x00\x00\x00\x60\x54\xee\x40"  // price
      "\x00\x00\x00\x00\x00\x00\x69\x40"  // amount
      "\x61\x37\x2d\x7c\x92\x01\x00\x00"  // timestamp ms
      "\xb8\x1e\x85\xeb\x19\x54\xee\x40"  // mark price
      "\x9a\x99\x99\x99\x69\x52\xee\x40"  // index price
      "\x4d\x93\x12\x0d\x00\x00\x00\x00"  // trade seq
      "\xdd\xc0\x1b\x13\x00\x00\x00\x00"  // trade id
      "\x03"                              // tick direction
      "\x00"                              // liquidation
      "\xff\xff\xff\xff\xff\xff\xff\xff"  // block trade id
      "\x00\x00\x00\x00\x00\x00\x00\x00"  // combo trade id
      "\x00\x00\x00\x00\x00\x00\x00\x00"  // (empty)
      // --- message header
      "\x85\x00"  // block length (133)
      "\xeb\x03"  // template id (1003)
      "\x01\x00"  // schema id (1)
      "\x02\x00"  // version (2)
      "\x00\x00"  // num groups (0)
      "\x00\x00"  // num var data fields (0)
      // --- ticker
      "\x96\x37\x03\x00"                  // instrument id
      "\x01"                              // instrument state
      "\x61\x37\x2d\x7c\x92\x01\x00\x00"  // timestamp ms
      "\x00\x00\x00\x52\xda\xe0\xc0\x41"  // open interest
      "\x00\x00\x00\x00\x20\x6b\xed\x40"  // min sell price
      "\x00\x00\x00\x00\x10\x3d\xef\x40"  // max buy price
      "\x00\x00\x00\x00\x60\x54\xee\x40"  // last price
      "\x9a\x99\x99\x99\x69\x52\xee\x40"  // index price
      "\xb8\x1e\x85\xeb\x19\x54\xee\x40"  // mark price
      "\x00\x00\x00\x00\x60\x54\xee\x40"  // best bid price
      "\x00\x00\x00\x00\x00\xd0\x8b\x40"  // best bid amount
      "\x00\x00\x00\x00\x70\x54\xee\x40"  // best ask price
      "\x00\x00\x00\x00\x80\x1a\x15\x41"  // best ask amount
      "\x00\x00\x00\x00\x00\x00\x00\x00"  // current funding
      "\xc3\xa0\x4c\xa3\xc9\xc5\x08\x3f"  // funding 8h
      "\x9a\x99\x99\x99\x69\x52\xee\x40"  // estimated delivery price
      "\xff\xff\xff\xff\xff\xff\xff\xff"  // delivery price
      "\xf6\x28\x5c\x8f\x5a\x99\xed\x40"  // settlement price
      // --- message header
      "\x1d\x00"  // block length (29)
      "\xe9\x03"  // template id (1001)
      "\x01\x00"  // schema id (1)
      "\x02\x00"  // version (2)
      "\x01\x00"  // num groups (1)
      "\x00\x00"  // num var data fields (0)
      // --- book
      "\x96\x37\x03\x00"                  // instrument id
      "\x61\x37\x2d\x7c\x92\x01\x00\x00"  // timestamp ms
      "\x14\xd7\x09\x60\x12\x00\x00\x00"  // prev change id
      "\x15\xd7\x09\x60\x12\x00\x00\x00"  // change id
      "\x01"                              // is last
      // -- changes
      "\x12\x00"  // block length (18)
      "\x01\x00"  // num in group (1)
      "\x00\x00"  // num groups (0)
      "\x00\x00"  // num var data field (0)
      // -- changes[0]
      "\x01"                                 // side
      "\x01"                                 // change
      "\x00\x00\x00\x00\x60\x54\xee\x40"     // price
      "\x00\x00\x00\x00\x00\xd0\x8b\x40"sv;  // amount

  struct MyHandler : public sbe::Parser::Handler {
    void finished() const {
      CHECK(frame_count_ == 1);
      CHECK(trades_count_ == 1);
      CHECK(ticker_count_ == 1);
      CHECK(book_count_ == 1);
    }

   protected:
    bool operator()(sbe::Frame const &frame) override {
      ++frame_count_;
      CHECK(frame.channel_id == 1);
      CHECK(frame.sequence_number == 50728558);
      return true;
    }
    void operator()(Trace<deribit_multicast::Instrument> const &, sbe::Frame const &) override { FAIL(); }
    void operator()(Trace<deribit_multicast::Book> const &event, sbe::Frame const &frame) override {
      ++book_count_;
      CHECK(frame.channel_id == 1);
      CHECK(frame.sequence_number == 50728558);
      auto &[trace_info, book] = event;
      CHECK(book.instrumentId() == 210838);
      CHECK(book.timestampMs() == 1728660191073);
      CHECK(book.prevChangeId() == 78920668948);
      CHECK(book.changeId() == 78920668949);
      CHECK(book.isLast() == true);
      using value_type = std::remove_cvref_t<decltype(book)>;
      const_cast<value_type &>(book).sbeRewind();  // note!
      size_t count = 0;
      const_cast<value_type &>(book).changesList().forEach([&count](auto &item) {
        switch (++count) {
          case 1:
            CHECK(item.side() == deribit_multicast::BookSide::bid);
            CHECK(item.change() == deribit_multicast::BookChange::changed);
            CHECK(item.price() == Catch::Approx{62115.0});
            CHECK(item.amount() == Catch::Approx{890.0});
            break;
        }
      });
      CHECK(count == 1);
    }
    void operator()(Trace<deribit_multicast::Trades> const &event, sbe::Frame const &frame) override {
      ++trades_count_;
      CHECK(frame.channel_id == 1);
      CHECK(frame.sequence_number == 50728558);
      auto &[trace_info, trades] = event;
      CHECK(trades.instrumentId() == 210838);
      using value_type = std::remove_cvref_t<decltype(trades)>;
      const_cast<value_type &>(trades).sbeRewind();  // note!
      size_t count = 0;
      const_cast<value_type &>(trades).tradesList().forEach([&count](auto &item) {
        switch (++count) {
          case 1:
            CHECK(item.direction() == deribit_multicast::Direction::sell);
            CHECK(item.price() == Catch::Approx{62115.0});
            CHECK(item.amount() == Catch::Approx{200.0});
            CHECK(item.timestampMs() == 1728660191073);
            CHECK(item.markPrice() == Catch::Approx{62112.80999999999767169});
            CHECK(item.indexPrice() == Catch::Approx{62099.30000000000291038});
            CHECK(item.tradeSeq() == 219321165);
            CHECK(item.tradeId() == 320585949);
            CHECK(item.tickDirection() == deribit_multicast::TickDirection::zerominus);
            CHECK(item.liquidation() == deribit_multicast::Liquidation::none);
            CHECK(std::isnan(item.iv()));
            CHECK(item.blockTradeId() == 0);
            CHECK(item.comboTradeId() == 0);
            break;
        }
      });
      CHECK(count == 1);
    }
    void operator()(Trace<deribit_multicast::Ticker> const &event, sbe::Frame const &frame) override {
      ++ticker_count_;
      CHECK(frame.channel_id == 1);
      CHECK(frame.sequence_number == 50728558);
      auto &[trace_info, ticker] = event;
      CHECK(ticker.instrumentId() == 210838);
      CHECK(ticker.instrumentState() == deribit_multicast::InstrumentState::open);
      CHECK(ticker.timestampMs() == 1728660191073);
      CHECK(ticker.openInterest() == Catch::Approx{566342820.0});
      CHECK(ticker.minSellPrice() == Catch::Approx{60249.0});
      CHECK(ticker.maxBuyPrice() == Catch::Approx{63976.5});
      CHECK(ticker.lastPrice() == Catch::Approx{62115.0});
      CHECK(ticker.indexPrice() == Catch::Approx{62099.30000000000291038});
      CHECK(ticker.markPrice() == Catch::Approx{62112.80999999999767169});
      CHECK(ticker.bestBidPrice() == Catch::Approx{62115.0});
      CHECK(ticker.bestBidAmount() == Catch::Approx{890.0});
      CHECK(ticker.bestAskPrice() == Catch::Approx{62115.5});
      CHECK(ticker.bestAskAmount() == Catch::Approx{345760.0});
      CHECK(ticker.currentFunding() == Catch::Approx{0.0});
      CHECK(ticker.funding8h() == Catch::Approx{0.00004725});
      CHECK(ticker.estimatedDeliveryPrice() == Catch::Approx{62099.30000000000291038});
      CHECK(std::isnan(ticker.deliveryPrice()));
      CHECK(ticker.settlementPrice() == Catch::Approx{60618.83000000000174623});
    }
    void operator()(Trace<deribit_multicast::Snapshot> const &, sbe::Frame const &) override { FAIL(); }
    void operator()(Trace<deribit_multicast::SnapshotStart> const &, sbe::Frame const &) override { FAIL(); }
    void operator()(Trace<deribit_multicast::SnapshotEnd> const &, sbe::Frame const &) override { FAIL(); }
    void operator()(Trace<deribit_multicast::ComboLegs> const &, sbe::Frame const &) override { FAIL(); }
    void operator()(Trace<deribit_multicast::PriceIndex> const &, sbe::Frame const &) override { FAIL(); }
    void operator()(Trace<deribit_multicast::Rfq> const &, sbe::Frame const &) override { FAIL(); }
    void operator()(Trace<deribit_multicast::InstrumentV2> const &, sbe::Frame const &) override { FAIL(); }

   private:
    size_t frame_count_ = {};
    size_t trades_count_ = {};
    size_t ticker_count_ = {};
    size_t book_count_ = {};
  } handler;

  REQUIRE(std::size(message) == 327);
  std::span buffer{reinterpret_cast<std::byte const *>(std::data(message)), std::size(message)};

  TraceInfo trace_info;
  auto result = sbe::Parser::dispatch(handler, buffer, trace_info);
  CHECK(result == true);
  handler.finished();
}

TEST_CASE("sbe_snapshot_1", "[sbe_parser]") {
  auto message =
      // --- frame
      "\xa4\x05"          // packet length (1444)
      "\x65\x00"          // channel (101)
      "\xb2\x71\x02\x00"  // sequence number
      // --- message header
      "\x8b\x00"  // block length (139)
      "\xf2\x03"  // template id (1010)
      "\x01\x00"  // schema id (1)
      "\x03\x00"  // version (3)
      "\x01\x00"  // num groups (1)
      "\x01\x00"  // num var data fields (1)
      // --- instrument v2
      "\x96\x37\x03\x00"                                      // instrument id
      "\x01"                                                  // instrument state
      "\x00"                                                  // kind
      "\x01"                                                  // instrument type
      "\x00"                                                  // option type
      "\x00"                                                  // settlement period
      "\x01\x00"                                              // settlement period count
      "\x42\x54\x43\x00\x00\x00\x00\x00"                      // base currency
      "\x55\x53\x44\x00\x00\x00\x00\x00"                      // quote currency
      "\x55\x53\x44\x00\x00\x00\x00\x00"                      // counter currency
      "\x42\x54\x43\x00\x00\x00\x00\x00"                      // settlement currency
      "\x55\x53\x44\x00\x00\x00\x00\x00"                      // size currency
      "\x98\x6d\xf7\x37\x65\x01\x00\x00"                      // creation timestamp ms
      "\x00\x54\x04\xdc\x8f\x1d\x00\x00"                      // expiration timestamp ms
      "\xff\xff\xff\xff\xff\xff\xff\xff"                      // strike price
      "\x00\x00\x00\x00\x00\x00\x24\x40"                      // contract size
      "\x00\x00\x00\x00\x00\x00\x24\x40"                      // min trade amount
      "\x00\x00\x00\x00\x00\x00\xe0\x3f"                      // tick size
      "\x00\x00\x00\x00\x00\x00\x00\x00"                      // maker commission
      "\xfc\xa9\xf1\xd2\x4d\x62\x40\x3f"                      // taker commission
      "\xfc\xa9\xf1\xd2\x4d\x62\x30\x3f"                      // block trade commission
      "\xb8\x1e\x85\xeb\x51\xb8\x7e\x3f"                      // max liquidation commission
      "\x00\x00\x00\x00\x00\x00\x49\x40"                      // max leverage
      "\x10\x00"                                              // block length
      "\x00\x00"                                              // num in group (0)
      "\x00\x00"                                              // num groups (0)
      "\x00\x00"                                              // num var data fields (0)
      "\x0d"                                                  // instrument name length (13)
      "\x42\x54\x43\x2d\x50\x45\x52\x50\x45\x54\x55\x41\x4c"  // instrument name (BTC-PERPETUAL)
      // --- message header
      "\x8c\x00"  // block length (140)
      "\xe8\x03"  // template id (1000)
      "\x01\x00"  // schema id (1)
      "\x02\x00"  // version (2)
      "\x00\x00"  // num groups (0)
      "\x01\x00"  // num var data fields (1)
      // --- instrument
      "\x96\x37\x03\x00"                                      // instrument id
      "\x01"                                                  // instrument state
      "\x00"                                                  // kind
      "\x01"                                                  // instrument type
      "\x00"                                                  // option type
      "\x00"                                                  // rfq
      "\x00"                                                  // settlement period
      "\x01\x00"                                              // settlement period count
      "\x42\x54\x43\x00\x00\x00\x00\x00"                      // base currency
      "\x55\x53\x44\x00\x00\x00\x00\x00"                      // quote currency
      "\x55\x53\x44\x00\x00\x00\x00\x00"                      // counter currency
      "\x42\x54\x43\x00\x00\x00\x00\x00"                      // settlement currency
      "\x55\x53\x44\x00\x00\x00\x00\x00"                      // size currency
      "\x98\x6d\xf7\x37\x65\x01\x00\x00"                      // creation timestamp ms
      "\x00\x54\x04\xdc\x8f\x1d\x00\x00"                      // expiration timestamp ms
      "\xff\xff\xff\xff\xff\xff\xff\xff"                      // strike price
      "\x00\x00\x00\x00\x00\x00\x24\x40"                      // contract size
      "\x00\x00\x00\x00\x00\x00\x24\x40"                      // min trade amount
      "\x00\x00\x00\x00\x00\x00\xe0\x3f"                      // tick size
      "\x00\x00\x00\x00\x00\x00\x00\x00"                      // maker commission
      "\xfc\xa9\xf1\xd2\x4d\x62\x40\x3f"                      // taker commission
      "\xfc\xa9\xf1\xd2\x4d\x62\x30\x3f"                      // block trade commission
      "\xb8\x1e\x85\xeb\x51\xb8\x7e\x3f"                      // max liquidation commission
      "\x00\x00\x00\x00\x00\x00\x49\x40"                      // max leverage
      "\x0d"                                                  // instrument name length (13)
      "\x42\x54\x43\x2d\x50\x45\x52\x50\x45\x54\x55\x41\x4c"  // instrument name (BTC-PERPETUAL)
      // --- message header
      "\x85\x00"  // block length
      "\xeb\x03"  // template id (1003)
      "\x01\x00"  // schema id (1)
      "\x02\x00"  // version (2)
      "\x00\x00"  // num groups (0)
      "\x00\x00"  // num var data fields (0)
      // --- ticker
      "\x96\x37\x03\x00"                  // instrument id
      "\x01"                              // instrument state
      "\x50\xc0\x2d\x7c\x92\x01\x00\x00"  // timestamp ms
      "\x00\x00\x00\x3b\xd3\xe0\xc0\x41"  // open interest
      "\x00\x00\x00\x00\x70\x67\xed\x40"  // min sell price
      "\x00\x00\x00\x00\x20\x39\xef\x40"  // max buy price
      "\x00\x00\x00\x00\x30\x50\xee\x40"  // last price
      "\xe1\x7a\x14\xae\x9f\x4e\xee\x40"  // index price
      "\x48\xe1\x7a\x14\x46\x50\xee\x40"  // mark price
      "\x00\x00\x00\x00\x20\x50\xee\x40"  // best bid price
      "\x00\x00\x00\x00\x80\xfd\xd3\x40"  // best bid amount
      "\x00\x00\x00\x00\x30\x50\xee\x40"  // best ask price
      "\x00\x00\x00\x00\x40\x09\xf6\x40"  // best ask amount
      "\x00\x00\x00\x00\x00\x00\x00\x00"  // current funding
      "\xc3\xa0\x4c\xa3\xc9\xc5\x08\x3f"  // funding 8h
      "\xe1\x7a\x14\xae\x9f\x4e\xee\x40"  // estimated delivery price
      "\xff\xff\xff\xff\xff\xff\xff\xff"  // delivery price
      "\xf6\x28\x5c\x8f\x5a\x99\xed\x40"  // settlement price
      // --- message header
      "\x16\x00"  // block length
      "\xec\x03"  // template id (1004)
      "\x01\x00"  // schema id (1)
      "\x02\x00"  // version (2)
      "\x01\x00"  // num groups (1)
      "\x00\x00"  // num var data fields (0)
      // --- snapshot
      "\x96\x37\x03\x00"                  // instrument id
      "\x50\xc0\x2d\x7c\x92\x01\x00\x00"  // timestamp ms
      "\x43\x58\x0a\x60\x12\x00\x00\x00"  // change id
      "\x01"                              // is book complete
      "\x00"                              // is last in book
      "\x11\x00"                          // block length (17)
      "\x36\x00"                          // num in group (54)
      "\x00\x00"                          // num groups (0)
      "\x00\x00"                          // num var data fields (0)
      // --- levels[0]
      "\x00"                              // book side
      "\x00\x00\x00\x00\x30\x50\xee\x40"  // price
      "\x00\x00\x00\x00\x40\x09\xf6\x40"  // amount
      // --- levels[1]
      "\x01"                              // book side
      "\x00\x00\x00\x00\x20\x50\xee\x40"  // price
      "\x00\x00\x00\x00\x80\xfd\xd3\x40"  // amount
      // --- levels[2...]
      "\x00\x00\x00\x00\x00\x40\x50\xee\x40\x00\x00\x00\x00\x00\x08\x96\x40"
      "\x01\x00\x00\x00\x00\x60\x4f\xee\x40\x00\x00\x00\x00\x00\x1d\xe0\x40"
      "\x00\x00\x00\x00\x00\x70\x50\xee\x40\x00\x00\x00\x00\x00\x88\xd3\x40"
      "\x01\x00\x00\x00\x00\x50\x4f\xee\x40\x00\x00\x00\x00\x00\x94\xe1\x40"
      "\x00\x00\x00\x00\x00\xc0\x50\xee\x40\x00\x00\x00\x00\x00\x40\xaf\x40"
      "\x01\x00\x00\x00\x00\xe0\x4e\xee\x40\x00\x00\x00\x00\x00\xd8\xad\x40"
      "\x00\x00\x00\x00\x00\x00\x51\xee\x40\x00\x00\x00\x00\x80\x2b\xe8\x40"
      "\x01\x00\x00\x00\x00\x80\x4e\xee\x40\x00\x00\x00\x00\x00\x94\xe1\x40"
      "\x00\x00\x00\x00\x00\x10\x51\xee\x40\x00\x00\x00\x00\xc0\x74\x01\x41"
      "\x01\x00\x00\x00\x00\x70\x4e\xee\x40\x00\x00\x00\x00\x00\x58\x96\x40"
      "\x00\x00\x00\x00\x00\x80\x51\xee\x40\x00\x00\x00\x00\x80\x48\xde\x40"
      "\x01\x00\x00\x00\x00\x60\x4e\xee\x40\x00\x00\x00\x00\x00\xac\xe7\x40"
      "\x00\x00\x00\x00\x00\xa0\x51\xee\x40\x00\x00\x00\x00\x00\x30\xa6\x40"
      "\x01\x00\x00\x00\x00\x50\x4e\xee\x40\x00\x00\x00\x00\x00\x16\xc7\x40"
      "\x00\x00\x00\x00\x00\xb0\x51\xee\x40\x00\x00\x00\x00\x00\x88\xd3\x40"
      "\x01\x00\x00\x00\x00\x40\x4e\xee\x40\x00\x00\x00\x00\x00\x6a\xe8\x40"
      "\x00\x00\x00\x00\x00\x50\x52\xee\x40\x00\x00\x00\x00\x20\xe3\xf5\x40"
      "\x01\x00\x00\x00\x00\x10\x4e\xee\x40\x00\x00\x00\x00\x00\x1d\xd0\x40"
      "\x00\x00\x00\x00\x00\x60\x52\xee\x40\x00\x00\x00\x00\x00\x12\xbb\x40"
      "\x01\x00\x00\x00\x00\x00\x4e\xee\x40\x00\x00\x00\x00\x00\x40\xba\x40"
      "\x00\x00\x00\x00\x00\x70\x52\xee\x40\x00\x00\x00\x00\x00\x00\x24\x40"
      "\x01\x00\x00\x00\x00\xf0\x4d\xee\x40\x00\x00\x00\x00\x00\x70\xa7\x40"
      "\x00\x00\x00\x00\x00\x80\x52\xee\x40\x00\x00\x00\x00\x00\x88\xb3\x40"
      "\x01\x00\x00\x00\x00\xd0\x4d\xee\x40\x00\x00\x00\x00\x00\x00\x24\x40"
      "\x00\x00\x00\x00\x00\x90\x52\xee\x40\x00\x00\x00\x00\x80\x1a\xd0\x40"
      "\x01\x00\x00\x00\x00\xb0\x4d\xee\x40\x00\x00\x00\x00\x00\x94\xe1\x40"
      "\x00\x00\x00\x00\x00\xb0\x52\xee\x40\x00\x00\x00\x00\x00\x00\x59\x40"
      "\x01\x00\x00\x00\x00\xa0\x4d\xee\x40\x00\x00\x00\x00\x00\x40\xaf\x40"
      "\x00\x00\x00\x00\x00\xc0\x52\xee\x40\x00\x00\x00\x00\x80\x1a\xd0\x40"
      "\x01\x00\x00\x00\x00\x90\x4d\xee\x40\x00\x00\x00\x00\x00\x00\x3e\x40"
      "\x00\x00\x00\x00\x00\xd0\x52\xee\x40\x00\x00\x00\x00\x00\x40\x8f\x40"
      "\x01\x00\x00\x00\x00\x70\x4d\xee\x40\x00\x00\x00\x00\x60\xdf\xf5\x40"
      "\x00\x00\x00\x00\x00\x10\x53\xee\x40\x00\x00\x00\x00\x00\xac\xa7\x40"
      "\x01\x00\x00\x00\x00\x60\x4d\xee\x40\x00\x00\x00\x00\x00\x6a\xe8\x40"
      "\x00\x00\x00\x00\x00\x40\x53\xee\x40\x00\x00\x00\x00\x00\x9a\xb0\x40"
      "\x01\x00\x00\x00\x00\x50\x4d\xee\x40\x00\x00\x00\x00\x40\xb8\x01\x41"
      "\x00\x00\x00\x00\x00\x50\x53\xee\x40\x00\x00\x00\x00\x00\x94\xe1\x40"
      "\x01\x00\x00\x00\x00\x20\x4d\xee\x40\x00\x00\x00\x00\x40\xba\xee\x40"
      "\x00\x00\x00\x00\x00\x70\x53\xee\x40\x00\x00\x00\x00\x80\x1a\xd0\x40"
      "\x01\x00\x00\x00\x00\xf0\x4c\xee\x40\x00\x00\x00\x00\x00\x7a\xb2\x40"
      "\x00\x00\x00\x00\x00\xa0\x53\xee\x40\x00\x00\x00\x00\x00\x6a\xe8\x40"
      "\x01\x00\x00\x00\x00\xe0\x4c\xee\x40\x00\x00\x00\x00\x00\x94\xe1\x40"
      "\x00\x00\x00\x00\x00\xe0\x53\xee\x40\x00\x00\x00\x00\x80\x1a\xd0\x40"
      "\x01\x00\x00\x00\x00\xd0\x4c\xee\x40\x00\x00\x00\x00\x00\x46\xd9\x40"
      "\x00\x00\x00\x00\x00\xf0\x53\xee\x40\x00\x00\x00\x00\x00\xa0\x7e\x40"
      "\x01\x00\x00\x00\x00\x90\x4c\xee\x40\x00\x00\x00\x00\x00\x58\xbb\x40"
      "\x00\x00\x00\x00\x00\x10\x54\xee\x40\x00\x00\x00\x00\x40\xff\xe0\x40"
      "\x01\x00\x00\x00\x00\x80\x4c\xee\x40\x00\x00\x00\x00\x00\xa1\xd8\x40"
      "\x00\x00\x00\x00\x00\x60\x54\xee\x40\x00\x00\x00\x00\x00\x94\xe1\x40"
      "\x01\x00\x00\x00\x00\x40\x4c\xee\x40\x00\x00\x00\x00\x00\xe0\xa5\x40"
      "\x00\x00\x00\x00\x00\x90\x54\xee\x40\x00\x00\x00\x00\xc0\x49\xee\x40"
      "\x01\x00\x00\x00\x00\x20\x4c\xee\x40\x00\x00\x00\x00\x00\x4b\xce\x40"sv;

  struct MyHandler : public sbe::Parser::Handler {
    void finished() const {
      CHECK(frame_count_ == 1);
      CHECK(instrument_count_ == 1);
      CHECK(ticker_count_ == 1);
      CHECK(snapshot_count_ == 1);
      CHECK(instrument_v2_count_ == 1);
    }

   protected:
    bool operator()(sbe::Frame const &frame) override {
      ++frame_count_;
      CHECK(frame.channel_id == 101);
      CHECK(frame.sequence_number == 160178);
      return true;
    }
    void operator()(Trace<deribit_multicast::Instrument> const &event, sbe::Frame const &frame) override {
      ++instrument_count_;
      CHECK(frame.channel_id == 101);
      CHECK(frame.sequence_number == 160178);
      auto &[trace_info, instrument] = event;
      using value_type = std::remove_cvref_t<decltype(instrument)>;
      CHECK(instrument.instrumentId() == 210838);
      CHECK(instrument.instrumentState() == deribit_multicast::InstrumentState::open);
      CHECK(instrument.kind() == deribit_multicast::InstrumentKind::future);
      CHECK(instrument.instrumentType() == deribit_multicast::InstrumentType::reversed);
      CHECK(instrument.optionType() == deribit_multicast::OptionType::not_applicable);
      CHECK(instrument.rfq() == deribit_multicast::YesNo::no);
      CHECK(instrument.settlementPeriod() == deribit_multicast::Period::perpetual);
      CHECK(instrument.settlementPeriodCount() == 1);
      CHECK(instrument.getBaseCurrencyAsStringView() == "BTC"sv);
      CHECK(instrument.getQuoteCurrencyAsStringView() == "USD"sv);
      CHECK(instrument.getCounterCurrencyAsStringView() == "USD"sv);
      CHECK(instrument.getSettlementCurrencyAsStringView() == "BTC"sv);
      CHECK(instrument.getSizeCurrencyAsStringView() == "USD"sv);
      CHECK(instrument.creationTimestampMs() == 1534242287000);
      CHECK(instrument.expirationTimestampMs() == 32503708800000);  // note!
      CHECK(std::isnan(instrument.strikePrice()));
      CHECK(instrument.contractSize() == Catch::Approx{10.0});
      CHECK(instrument.minTradeAmount() == Catch::Approx{10.0});
      CHECK(instrument.tickSize() == Catch::Approx{0.5});
      CHECK(instrument.makerCommission() == Catch::Approx{0.0});
      CHECK(instrument.takerCommission() == Catch::Approx{0.0005});
      CHECK(instrument.blockTradeCommission() == Catch::Approx{0.00025});
      CHECK(instrument.maxLiquidationCommission() == Catch::Approx{0.0075});
      CHECK(instrument.maxLeverage() == Catch::Approx{50.0});
      CHECK(sbe::get_instrument_name(const_cast<value_type &>(instrument)) == "BTC-PERPETUAL"sv);
    }
    void operator()(Trace<deribit_multicast::Book> const &, sbe::Frame const &) override { FAIL(); }
    void operator()(Trace<deribit_multicast::Trades> const &, sbe::Frame const &) override { FAIL(); }
    void operator()(Trace<deribit_multicast::Ticker> const &event, sbe::Frame const &frame) override {
      ++ticker_count_;
      CHECK(frame.channel_id == 101);
      CHECK(frame.sequence_number == 160178);
      auto &[trace_info, ticker] = event;
      CHECK(ticker.instrumentId() == 210838);
      CHECK(ticker.instrumentState() == deribit_multicast::InstrumentState::open);
      CHECK(ticker.timestampMs() == 1728660226128);
      CHECK(ticker.openInterest() == Catch::Approx{566342820.0});
      CHECK(ticker.minSellPrice() == Catch::Approx{60219.5});
      CHECK(ticker.maxBuyPrice() == Catch::Approx{63945.0});
      CHECK(ticker.lastPrice() == Catch::Approx{62081.5});
      CHECK(ticker.indexPrice() == Catch::Approx{62068.98999999999796273});
      CHECK(ticker.markPrice() == Catch::Approx{62082.19000000000232831});
      CHECK(ticker.bestBidPrice() == Catch::Approx{62081.0});
      CHECK(ticker.bestBidAmount() == Catch::Approx{20470.0});
      CHECK(ticker.bestAskPrice() == Catch::Approx{62081.5});
      CHECK(ticker.bestAskAmount() == Catch::Approx{90260.0});
      CHECK(ticker.currentFunding() == Catch::Approx{0.0});
      CHECK(ticker.funding8h() == Catch::Approx{0.00004725});
      CHECK(ticker.estimatedDeliveryPrice() == Catch::Approx{62068.98999999999796273});
      CHECK(std::isnan(ticker.deliveryPrice()));
      CHECK(ticker.settlementPrice() == Catch::Approx{60618.83000000000174623});
    }
    void operator()(Trace<deribit_multicast::Snapshot> const &event, sbe::Frame const &frame) override {
      ++snapshot_count_;
      CHECK(frame.channel_id == 101);
      CHECK(frame.sequence_number == 160178);
      auto &[trace_info, snapshot] = event;
      using value_type = std::remove_cvref_t<decltype(snapshot)>;
      CHECK(snapshot.instrumentId() == 210838);
      size_t count = 0;
      const_cast<value_type &>(snapshot).levelsList().forEach([&count](auto &item) {
        switch (++count) {
          case 1:
            CHECK(item.side() == deribit_multicast::BookSide::ask);
            CHECK(item.price() == Catch::Approx{62081.5});
            CHECK(item.amount() == Catch::Approx{90260.0});
            break;
          case 2:
            CHECK(item.side() == deribit_multicast::BookSide::bid);
            CHECK(item.price() == Catch::Approx{62081.0});
            CHECK(item.amount() == Catch::Approx{20470.0});
            break;
          case 54:
            CHECK(item.side() == deribit_multicast::BookSide::bid);
            CHECK(item.price() == Catch::Approx{62049.0});
            CHECK(item.amount() == Catch::Approx{15510.0});
            break;
        }
      });
      CHECK(count == 54);
    }
    void operator()(Trace<deribit_multicast::SnapshotStart> const &, sbe::Frame const &) override { FAIL(); }
    void operator()(Trace<deribit_multicast::SnapshotEnd> const &, sbe::Frame const &) override { FAIL(); }
    void operator()(Trace<deribit_multicast::ComboLegs> const &, sbe::Frame const &) override { FAIL(); }
    void operator()(Trace<deribit_multicast::PriceIndex> const &, sbe::Frame const &) override { FAIL(); }
    void operator()(Trace<deribit_multicast::Rfq> const &, sbe::Frame const &) override { FAIL(); }
    void operator()(Trace<deribit_multicast::InstrumentV2> const &event, sbe::Frame const &frame) override {
      ++instrument_v2_count_;
      CHECK(frame.channel_id == 101);
      CHECK(frame.sequence_number == 160178);
      auto &[trace_info, instrument] = event;
      using value_type = std::remove_cvref_t<decltype(instrument)>;
      CHECK(instrument.instrumentId() == 210838);
      CHECK(instrument.instrumentState() == deribit_multicast::InstrumentState::open);
      CHECK(instrument.kind() == deribit_multicast::InstrumentKind::future);
      CHECK(instrument.instrumentType() == deribit_multicast::InstrumentType::reversed);
      CHECK(instrument.optionType() == deribit_multicast::OptionType::not_applicable);
      CHECK(instrument.settlementPeriod() == deribit_multicast::Period::perpetual);
      CHECK(instrument.settlementPeriodCount() == 1);
      CHECK(instrument.getBaseCurrencyAsStringView() == "BTC"sv);
      CHECK(instrument.getQuoteCurrencyAsStringView() == "USD"sv);
      CHECK(instrument.getCounterCurrencyAsStringView() == "USD"sv);
      CHECK(instrument.getSettlementCurrencyAsStringView() == "BTC"sv);
      CHECK(instrument.getSizeCurrencyAsStringView() == "USD"sv);
      CHECK(instrument.creationTimestampMs() == 1534242287000);
      CHECK(instrument.expirationTimestampMs() == 32503708800000);  // note!
      CHECK(std::isnan(instrument.strikePrice()));
      CHECK(instrument.contractSize() == Catch::Approx{10.0});
      CHECK(instrument.minTradeAmount() == Catch::Approx{10.0});
      CHECK(instrument.tickSize() == Catch::Approx{0.5});
      CHECK(instrument.makerCommission() == Catch::Approx{0.0});
      CHECK(instrument.takerCommission() == Catch::Approx{0.0005});
      CHECK(instrument.blockTradeCommission() == Catch::Approx{0.00025});
      CHECK(instrument.maxLiquidationCommission() == Catch::Approx{0.0075});
      CHECK(instrument.maxLeverage() == Catch::Approx{50.0});
      size_t count = 0;
      const_cast<value_type &>(instrument).tickStepsList().forEach([&count]([[maybe_unused]] auto &item) { ++count; });
      CHECK(count == 0);
      CHECK(instrument.instrumentNameLength() == 13);
      CHECK(sbe::get_instrument_name(const_cast<value_type &>(instrument)) == "BTC-PERPETUAL"sv);
    }

   private:
    size_t frame_count_ = {};
    size_t instrument_count_ = {};
    size_t ticker_count_ = {};
    size_t snapshot_count_ = {};
    size_t instrument_v2_count_ = {};
  } handler;

  REQUIRE(std::size(message) == 1452);
  std::span buffer{reinterpret_cast<std::byte const *>(std::data(message)), std::size(message)};

  TraceInfo trace_info;
  auto result = sbe::Parser::dispatch(handler, buffer, trace_info);
  CHECK(result == true);
  handler.finished();
}

TEST_CASE("sbe_snapshot_2", "[sbe_parser]") {
  auto message =
      "\x9f\x05"          // packet length (1439)
      "\x66\x00"          // channel id (102)
      "\x32\x5a\x8e\x00"  // sequence number
      // --- message header
      "\x8b\x00"  // block length (139)
      "\xf2\x03"  // template id (1010)
      "\x01\x00"  // schema id (1)
      "\x03\x00"  // version (3)
      "\x01\x00"  // num groups (1)
      "\x01\x00"  // num var data fields (1)
      // --- instrument v2
      "\xbd\x6f\x05\x00"                  // instrument id
      "\x01"                              // instrument state
      "\x01"                              // kind
      "\x01"                              // instrument type
      "\x02"                              // option type
      "\x05"                              // settlement period
      "\x01\x00"                          // settlement period count
      "\x42\x54\x43\x00\x00\x00\x00\x00"  // base currency
      "\x42\x54\x43\x00\x00\x00\x00\x00"  // quote currency
      "\x55\x53\x44\x00\x00\x00\x00\x00"  // counter currency
      "\x42\x54\x43\x00\x00\x00\x00\x00"  // settlement currency
      "\x42\x54\x43\x00\x00\x00\x00\x00"  // size currency
      "\xf8\xaf\x49\x21\x91\x01\x00\x00"  // creation timestamp ms
      "\x00\x80\xb0\xc2\x92\x01\x00\x00"  // expiration timestamp ms
      "\x00\x00\x00\x00\x00\xf3\xe6\x40"  // strike price
      "\x00\x00\x00\x00\x00\x00\xf0\x3f"  // contract size
      "\x9a\x99\x99\x99\x99\x99\xb9\x3f"  // min trade amount
      "\x2d\x43\x1c\xeb\xe2\x36\x1a\x3f"  // tick size
      "\x61\x32\x55\x30\x2a\xa9\x33\x3f"  // maker commission
      "\x61\x32\x55\x30\x2a\xa9\x33\x3f"  // taker commission
      "\x61\x32\x55\x30\x2a\xa9\x33\x3f"  // block trade commission
      "\xff\xff\xff\xff\xff\xff\xff\xff"  // max liquidation commission
      "\xff\xff\xff\xff\xff\xff\xff\xff"  // max leverage
      // --- tick steps
      "\x10\x00"  // block length
      "\x01\x00"  // num in group (1)
      "\x00\x00"  // num groups (0)
      "\x00\x00"  // num var data fields (0)
      // --- tick steps[0]
      "\x7b\x14\xae\x47\xe1\x7a\x74\x3f"  // above price
      "\xfc\xa9\xf1\xd2\x4d\x62\x40\x3f"  // tick size
      // --- instrument name
      "\x13"  // instrument name length (19)
      "\x42\x54\x43\x2d\x32\x35\x4f\x43\x54\x32\x34\x2d\x34\x37\x30\x30"
      "\x30\x2d\x50"  // instrument name (BTC-25OCT24-47000-P)
      // --- message header
      "\x8c\x00"  // block length (104)
      "\xe8\x03"  // template id (1000)
      "\x01\x00"  // schema id (1)
      "\x02\x00"  // version (2)
      "\x00\x00"  // num groups (1)
      "\x01\x00"  // num var data fields (1)
      // --- instrument
      "\xbd\x6f\x05\x00"                  // instrument id
      "\x01"                              // instrument state
      "\x01"                              // kind
      "\x01"                              // instrument type
      "\x02"                              // option type
      "\x00"                              // rfq
      "\x05"                              // settlement period
      "\x01\x00"                          // settlement period count
      "\x42\x54\x43\x00\x00\x00\x00\x00"  // base currency
      "\x42\x54\x43\x00\x00\x00\x00\x00"  // quote currency
      "\x55\x53\x44\x00\x00\x00\x00\x00"  // counter currency
      "\x42\x54\x43\x00\x00\x00\x00\x00"  // settlement currency
      "\x42\x54\x43\x00\x00\x00\x00\x00"  // size currency
      "\xf8\xaf\x49\x21\x91\x01\x00\x00"  // creation timestamp ms
      "\x00\x80\xb0\xc2\x92\x01\x00\x00"  // expiration timestamp ms
      "\x00\x00\x00\x00\x00\xf3\xe6\x40"  // strike price
      "\x00\x00\x00\x00\x00\x00\xf0\x3f"  // contract size
      "\x9a\x99\x99\x99\x99\x99\xb9\x3f"  // min trade amount
      "\x2d\x43\x1c\xeb\xe2\x36\x1a\x3f"  // tick size
      "\x61\x32\x55\x30\x2a\xa9\x33\x3f"  // maker commission
      "\x61\x32\x55\x30\x2a\xa9\x33\x3f"  // taker commission
      "\x61\x32\x55\x30\x2a\xa9\x33\x3f"  // block trade commission
      "\xff\xff\xff\xff\xff\xff\xff\xff"  // max liquidation commission
      "\xff\xff\xff\xff\xff\xff\xff\xff"  // max leverage
      "\x13"                              // instrument length (19)
      "\x42\x54\x43\x2d\x32\x35\x4f\x43\x54\x32\x34\x2d\x34\x37\x30\x30"
      "\x30\x2d\x50"  // instrument name (BTC-25OCT24-47000-P)
      // --- message header
      "\x85\x00"  // block length
      "\xeb\x03"  // template id (1003)
      "\x01\x00"
      "\x02\x00"
      "\x00\x00"
      "\x00\x00"
      "\xbd\x6f\x05\x00\x01\x1c\x51\x66\xb8\x92\x01\x00\x00\x66\x66\x66\x66\x66\x56\x82\x40\x2d\x43\x1c\xeb\xe2\x36\x1a\x3f\xb8\x1e\x85\xeb\x51\xb8\x8e\x3f\x2d"
      "\x43\x1c\xeb\xe2\x36\x1a\x3f\x52\xb8\x1e\x85\x47\x4e\xf0\x40\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
      "\x00\x2d\x43\x1c\xeb\xe2\x36\x1a\x3f\x00\x00\x00\x00\x00\x00\x14\x40\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x52\xb8\x1e\x85\x47"
      "\x4e\xf0\x40\xff\xff\xff\xff\xff\xff\xff\xff\x3a\x8c\x30\xe2\x8e\x79\x95\x3e"
      // --- message header
      "\x16\x00"  // block length
      "\xec\x03"  // template id (1004)
      "\x01\x00"
      "\x02\x00"
      "\x01\x00"
      "\x00\x00"
      "\xbd\x6f\x05\x00\x1c\x51\x66\xb8\x92\x01\x00\x00\x0b\x4a\xd3\x7f\x12\x00\x00\x00\x01\x01\x11\x00\x06\x00\x00\x00\x00\x00\x00\x2d\x43\x1c\xeb\xe2\x36\x1a"
      "\x3f\x00\x00\x00\x00\x00\x00\x14\x40\x00\x61\x32\x55\x30\x2a\xa9\x33\x3f\x33\x33\x33\x33\x33\xb3\x33\x40\x00\x2d\x43\x1c\xeb\xe2\x36\x3a\x3f\x00\x00\x00"
      "\x00\x00\x00\xf0\x3f\x00\x79\xe9\x26\x31\x08\xac\x6c\x3f\x9a\x99\x99\x99\x99\x99\xb9\x3f\x00\x15\x8c\x4a\xea\x04\x34\x71\x3f\x00\x00\x00\x00\x00\x00\x35"
      "\x40\x00\x7b\x14\xae\x47\xe1\x7a\x84\x3f\x33\x33\x33\x33\x33\x33\xd3\x3f"
      // --- message header
      "\x8b\x00"  // block length
      "\xf2\x03"  // template id (1010)
      "\x01\x00"
      "\x03\x00"
      "\x01\x00"
      "\x01\x00"
      "\x48\x59\x04\x00\x01\x01\x01\x01\x05\x03\x00\x42\x54\x43\x00\x00\x00\x00\x00\x42\x54\x43\x00\x00\x00\x00\x00\x55\x53\x44\x00\x00\x00\x00\x00\x42\x54\x43"
      "\x00\x00\x00\x00\x00\x42\x54\x43\x00\x00\x00\x00\x00\x28\x1b\x70\xaf\x8c\x01\x00\x00\x00\x24\x21\x07\x94\x01\x00\x00\x00\x00\x00\x00\x00\x4c\xcd\x40\x00"
      "\x00\x00\x00\x00\x00\xf0\x3f\x9a\x99\x99\x99\x99\x99\xb9\x3f\x2d\x43\x1c\xeb\xe2\x36\x1a\x3f\x61\x32\x55\x30\x2a\xa9\x33\x3f\x61\x32\x55\x30\x2a\xa9\x33"
      "\x3f\x61\x32\x55\x30\x2a\xa9\x33\x3f\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x10\x00\x01\x00\x00\x00\x00\x00\x7b\x14\xae\x47\xe1"
      "\x7a\x74\x3f\xfc\xa9\xf1\xd2\x4d\x62\x40\x3f\x13\x42\x54\x43\x2d\x32\x37\x44\x45\x43\x32\x34\x2d\x31\x35\x30\x30\x30\x2d\x43"
      // --- message header
      "\x8c\x00"  // block length
      "\xe8\x03"  // template id (1000)
      "\x01\x00"
      "\x02\x00"
      "\x00\x00"
      "\x01\x00"
      "\x48\x59\x04\x00\x01\x01\x01\x01\x00\x05\x03\x00\x42\x54\x43\x00\x00\x00\x00\x00\x42\x54\x43\x00\x00\x00\x00\x00\x55\x53\x44\x00\x00\x00\x00\x00\x42\x54"
      "\x43\x00\x00\x00\x00\x00\x42\x54\x43\x00\x00\x00\x00\x00\x28\x1b\x70\xaf\x8c\x01\x00\x00\x00\x24\x21\x07\x94\x01\x00\x00\x00\x00\x00\x00\x00\x4c\xcd\x40"
      "\x00\x00\x00\x00\x00\x00\xf0\x3f\x9a\x99\x99\x99\x99\x99\xb9\x3f\x2d\x43\x1c\xeb\xe2\x36\x1a\x3f\x61\x32\x55\x30\x2a\xa9\x33\x3f\x61\x32\x55\x30\x2a\xa9"
      "\x33\x3f\x61\x32\x55\x30\x2a\xa9\x33\x3f\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x13\x42\x54\x43\x2d\x32\x37\x44\x45\x43\x32\x34"
      "\x2d\x31\x35\x30\x30\x30\x2d\x43"
      // --- message header
      "\x85\x00"  // block length
      "\xeb\x03"  // template id (1003)
      "\x01\x00"
      "\x02\x00"
      "\x00\x00"
      "\x00\x00"
      "\x48\x59\x04\x00\x01\x75\x4d\x66\xb8\x92\x01\x00\x00\x66\x66\x66\x66\x66\x66\x20\x40\x7b\x14\xae\x47\xe1\x7a\xe8\x3f\x06\x81\x95\x43\x8b\x6c\xe9\x3f\xa6"
      "\x9b\xc4\x20\xb0\x72\xe8\x3f\xf6\x28\x5c\x8f\x4e\x4e\xf0\x40\x6d\xc5\xfe\xb2\x7b\xf2\xe8\x3f\x4c\x37\x89\x41\x60\xe5\xe8\x3f\xcd\xcc\xcc\xcc\xcc\xcc\x3e"
      "\x40\x35\x5e\xba\x49\x0c\x02\xe9\x3f\xcd\xcc\xcc\xcc\xcc\xcc\x3e\x40\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xf6\x28\x5c\x8f\x4e"
      "\x4e\xf0\x40\xff\xff\xff\xff\xff\xff\xff\xff\x84\xe1\x33\x44\xdb\xf2\xe8\x3f"
      // --- message header
      "\x16\x00"  // block length
      "\xec\x03"  // template id (1004)
      "\x01\x00"
      "\x02\x00"
      "\x01\x00"
      "\x00\x00"
      "\x48\x59\x04\x00\x75\x4d\x66\xb8\x92\x01\x00\x00\x45\x2c\xd4\x7f\x12\x00\x00\x00\x01\x01\x11\x00\x02\x00\x00\x00\x00\x00\x00\x35\x5e\xba\x49\x0c\x02\xe9"
      "\x3f\xcd\xcc\xcc\xcc\xcc\xcc\x3e\x40\x01\x4c\x37\x89\x41\x60\xe5\xe8\x3f\xcd\xcc\xcc\xcc\xcc\xcc\x3e\x40"
      // --- message header
      "\x8b\x00"  // block length
      "\xf2\x03"  // template id (1010)
      "\x01\x00"
      "\x03\x00"
      "\x01\x00"
      "\x01\x00"
      "\x61\x68\x05\x00\x01\x01\x01\x01\x05\x01\x00\x42\x54\x43\x00\x00\x00\x00\x00\x42\x54\x43\x00\x00\x00\x00\x00\x55\x53\x44\x00\x00\x00\x00\x00\x42\x54\x43\x00\x00\x00\x00\x00\x42\x54\x43\x00\x00\x00\x00\x00\x00\x83\x62\x0f\x91\x01\x00\x00\x00\x80\xb0\xc2\x92\x01\x00\x00\x00\x00\x00\x00\x00\xd5\xeb\x40\x00\x00\x00\x00\x00\x00\xf0\x3f\x9a\x99\x99\x99\x99\x99\xb9\x3f\x2d\x43\x1c\xeb\xe2\x36\x1a\x3f\x61\x32\x55\x30\x2a\xa9\x33\x3f\x61\x32\x55\x30\x2a\xa9\x33\x3f\x61\x32\x55\x30\x2a\xa9\x33\x3f\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x10\x00\x01\x00\x00\x00\x00\x00\x7b\x14\xae\x47\xe1\x7a\x74\x3f\xfc\xa9\xf1\xd2\x4d\x62\x40\x3f\x13\x42\x54\x43\x2d\x32\x35\x4f\x43\x54\x32\x34\x2d\x35\x37\x30\x30\x30\x2d\x43"sv;

  struct MyHandler : public sbe::Parser::Handler {
    void finished() const {
      CHECK(frame_count_ == 1);
      CHECK(instrument_count_ == 2);
      CHECK(ticker_count_ == 2);
      CHECK(snapshot_count_ == 2);
      CHECK(instrument_v2_count_ == 3);
    }

   protected:
    bool operator()(sbe::Frame const &frame) override {
      ++frame_count_;
      CHECK(frame.channel_id == 102);
      CHECK(frame.sequence_number == 9329202);
      return true;
    }
    void operator()(Trace<deribit_multicast::Instrument> const &event, sbe::Frame const &frame) override {
      CHECK(frame.channel_id == 102);
      CHECK(frame.sequence_number == 9329202);
      auto &[trace_info, instrument] = event;
      using value_type = std::remove_cvref_t<decltype(instrument)>;
      switch (++instrument_count_) {
        case 1: {
          CHECK(instrument.instrumentId() == 356285);
          CHECK(instrument.instrumentState() == deribit_multicast::InstrumentState::open);
          CHECK(instrument.kind() == deribit_multicast::InstrumentKind::option);
          CHECK(instrument.instrumentType() == deribit_multicast::InstrumentType::reversed);
          CHECK(instrument.optionType() == deribit_multicast::OptionType::put);
          CHECK(instrument.settlementPeriod() == deribit_multicast::Period::month);
          CHECK(instrument.settlementPeriodCount() == 1);
          CHECK(instrument.getBaseCurrencyAsStringView() == "BTC"sv);
          CHECK(instrument.getQuoteCurrencyAsStringView() == "BTC"sv);
          CHECK(instrument.getCounterCurrencyAsStringView() == "USD"sv);
          CHECK(instrument.getSettlementCurrencyAsStringView() == "BTC"sv);
          CHECK(instrument.getSizeCurrencyAsStringView() == "BTC"sv);
          CHECK(instrument.creationTimestampMs() == 1722840363000);
          CHECK(instrument.expirationTimestampMs() == 1729843200000);
          CHECK(instrument.strikePrice() == Catch::Approx{47000.0});
          CHECK(instrument.contractSize() == Catch::Approx{1.0});
          CHECK(instrument.minTradeAmount() == Catch::Approx{0.1});
          CHECK(instrument.tickSize() == Catch::Approx{0.0001});
          CHECK(instrument.makerCommission() == Catch::Approx{0.0003});
          CHECK(instrument.takerCommission() == Catch::Approx{0.0003});
          CHECK(instrument.blockTradeCommission() == Catch::Approx{0.0003});
          CHECK(std::isnan(instrument.maxLiquidationCommission()));
          CHECK(std::isnan(instrument.maxLeverage()));
          CHECK(instrument.instrumentNameLength() == 19);
          CHECK(sbe::get_instrument_name(const_cast<value_type &>(instrument)) == "BTC-25OCT24-47000-P"sv);
          break;
        }
        case 2: {
          CHECK(instrument.instrumentId() == 285000);
          CHECK(instrument.instrumentState() == deribit_multicast::InstrumentState::open);
          CHECK(instrument.kind() == deribit_multicast::InstrumentKind::option);
          CHECK(instrument.instrumentType() == deribit_multicast::InstrumentType::reversed);
          CHECK(instrument.optionType() == deribit_multicast::OptionType::call);
          CHECK(instrument.settlementPeriod() == deribit_multicast::Period::month);
          CHECK(instrument.settlementPeriodCount() == 3);
          CHECK(instrument.getBaseCurrencyAsStringView() == "BTC"sv);
          CHECK(instrument.getQuoteCurrencyAsStringView() == "BTC"sv);
          CHECK(instrument.getCounterCurrencyAsStringView() == "USD"sv);
          CHECK(instrument.getSettlementCurrencyAsStringView() == "BTC"sv);
          CHECK(instrument.getSizeCurrencyAsStringView() == "BTC"sv);
          CHECK(instrument.creationTimestampMs() == 1703750409000);
          CHECK(instrument.expirationTimestampMs() == 1735286400000);
          CHECK(instrument.strikePrice() == Catch::Approx{15000.0});
          CHECK(instrument.contractSize() == Catch::Approx{1.0});
          CHECK(instrument.minTradeAmount() == Catch::Approx{0.1});
          CHECK(instrument.tickSize() == Catch::Approx{0.0001});
          CHECK(instrument.makerCommission() == Catch::Approx{0.0003});
          CHECK(instrument.takerCommission() == Catch::Approx{0.0003});
          CHECK(instrument.blockTradeCommission() == Catch::Approx{0.0003});
          CHECK(std::isnan(instrument.maxLiquidationCommission()));
          CHECK(std::isnan(instrument.maxLeverage()));
          CHECK(instrument.instrumentNameLength() == 19);
          CHECK(sbe::get_instrument_name(const_cast<value_type &>(instrument)) == "BTC-27DEC24-15000-C"sv);
          break;
        }
        default:
          FAIL();
      }
    }
    void operator()(Trace<deribit_multicast::Book> const &, sbe::Frame const &) override { FAIL(); }
    void operator()(Trace<deribit_multicast::Trades> const &, sbe::Frame const &) override { FAIL(); }
    void operator()(Trace<deribit_multicast::Ticker> const &event, sbe::Frame const &frame) override {
      CHECK(frame.channel_id == 102);
      CHECK(frame.sequence_number == 9329202);
      auto &[trace_info, ticker] = event;
      switch (++ticker_count_) {
        case 1: {
          CHECK(ticker.instrumentId() == 356285);
          CHECK(ticker.instrumentState() == deribit_multicast::InstrumentState::open);
          CHECK(ticker.timestampMs() == 1729670566172);
          CHECK(ticker.openInterest() == Catch::Approx{586.79999999999995453});
          CHECK(ticker.minSellPrice() == Catch::Approx{0.0001});
          CHECK(ticker.maxBuyPrice() == Catch::Approx{0.015});
          CHECK(ticker.lastPrice() == Catch::Approx{0.0001});
          CHECK(ticker.indexPrice() == Catch::Approx{66788.47000000000116415});
          CHECK(ticker.markPrice() == Catch::Approx{0.0});
          CHECK(ticker.bestBidPrice() == Catch::Approx{0.0});
          CHECK(ticker.bestBidAmount() == Catch::Approx{0.0});
          CHECK(ticker.bestAskPrice() == Catch::Approx{0.0001});
          CHECK(ticker.bestAskAmount() == Catch::Approx{5.0});
          CHECK(std::isnan(ticker.currentFunding()));
          CHECK(std::isnan(ticker.funding8h()));
          CHECK(ticker.estimatedDeliveryPrice() == Catch::Approx{66788.47000000000116415});
          CHECK(std::isnan(ticker.deliveryPrice()));
          CHECK(ticker.settlementPrice() == Catch::Approx{0.00000032});
          break;
        }
        case 2: {
          CHECK(ticker.instrumentId() == 285000);
          CHECK(ticker.instrumentState() == deribit_multicast::InstrumentState::open);
          CHECK(ticker.timestampMs() == 1729670565237);
          CHECK(ticker.openInterest() == Catch::Approx{8.19999999999999929});
          CHECK(ticker.minSellPrice() == Catch::Approx{0.76500000000000001});
          CHECK(ticker.maxBuyPrice() == Catch::Approx{0.79449999999999998});
          CHECK(ticker.lastPrice() == Catch::Approx{0.76400000000000001});
          CHECK(ticker.indexPrice() == Catch::Approx{66788.91000000000349246});
          CHECK(ticker.markPrice() == Catch::Approx{0.77959999999999996});
          CHECK(ticker.bestBidPrice() == Catch::Approx{0.77800000000000002});
          CHECK(ticker.bestBidAmount() == Catch::Approx{30.80000000000000071});
          CHECK(ticker.bestAskPrice() == Catch::Approx{0.78149999999999997});
          CHECK(ticker.bestAskAmount() == Catch::Approx{30.80000000000000071});
          CHECK(std::isnan(ticker.currentFunding()));
          CHECK(std::isnan(ticker.funding8h()));
          CHECK(ticker.estimatedDeliveryPrice() == Catch::Approx{66788.47000000000116415});
          CHECK(std::isnan(ticker.deliveryPrice()));
          CHECK(ticker.settlementPrice() == Catch::Approx{0.77964557000000001});
          break;
        }
        default:
          FAIL();
      }
    }
    void operator()(Trace<deribit_multicast::Snapshot> const &event, sbe::Frame const &frame) override {
      CHECK(frame.channel_id == 102);
      CHECK(frame.sequence_number == 9329202);
      auto &[trace_info, snapshot] = event;
      using value_type = std::remove_cvref_t<decltype(snapshot)>;
      switch (++snapshot_count_) {
        case 1: {
          CHECK(snapshot.instrumentId() == 356285);
          size_t count = 0;
          const_cast<value_type &>(snapshot).levelsList().forEach([&count](auto &item) {
            switch (++count) {
              case 1:
                CHECK(item.side() == deribit_multicast::BookSide::ask);
                CHECK(item.price() == Catch::Approx{0.0001});
                CHECK(item.amount() == Catch::Approx{5.0});
                break;
              case 2:
                CHECK(item.side() == deribit_multicast::BookSide::ask);
                CHECK(item.price() == Catch::Approx{0.0003});
                CHECK(item.amount() == Catch::Approx{19.69999999999999929});
                break;
              case 6:
                CHECK(item.side() == deribit_multicast::BookSide::ask);
                CHECK(item.price() == Catch::Approx{0.01});
                CHECK(item.amount() == Catch::Approx{0.29999999999999999});
                break;
            }
          });
          CHECK(count == 6);
          break;
        }
        case 2: {
          CHECK(snapshot.instrumentId() == 285000);
          size_t count = 0;
          const_cast<value_type &>(snapshot).levelsList().forEach([&count](auto &item) {
            switch (++count) {
              case 1:
                CHECK(item.side() == deribit_multicast::BookSide::ask);
                CHECK(item.price() == Catch::Approx{0.78149999999999997});
                CHECK(item.amount() == Catch::Approx{30.80000000000000071});
                break;
              case 2:
                CHECK(item.side() == deribit_multicast::BookSide::bid);
                CHECK(item.price() == Catch::Approx{0.77800000000000002});
                CHECK(item.amount() == Catch::Approx{30.80000000000000071});
                break;
            }
          });
          CHECK(count == 2);
          break;
        }
        default:
          FAIL();
      }
    }
    void operator()(Trace<deribit_multicast::SnapshotStart> const &, sbe::Frame const &) override { FAIL(); }
    void operator()(Trace<deribit_multicast::SnapshotEnd> const &, sbe::Frame const &) override { FAIL(); }
    void operator()(Trace<deribit_multicast::ComboLegs> const &, sbe::Frame const &) override { FAIL(); }
    void operator()(Trace<deribit_multicast::PriceIndex> const &, sbe::Frame const &) override { FAIL(); }
    void operator()(Trace<deribit_multicast::Rfq> const &, sbe::Frame const &) override { FAIL(); }
    void operator()(Trace<deribit_multicast::InstrumentV2> const &event, sbe::Frame const &frame) override {
      CHECK(frame.channel_id == 102);
      CHECK(frame.sequence_number == 9329202);
      auto &[trace_info, instrument] = event;
      using value_type = std::remove_cvref_t<decltype(instrument)>;
      switch (++instrument_v2_count_) {
        case 1: {
          CHECK(instrument.instrumentId() == 356285);
          CHECK(instrument.instrumentState() == deribit_multicast::InstrumentState::open);
          CHECK(instrument.kind() == deribit_multicast::InstrumentKind::option);
          CHECK(instrument.instrumentType() == deribit_multicast::InstrumentType::reversed);
          CHECK(instrument.optionType() == deribit_multicast::OptionType::put);
          CHECK(instrument.settlementPeriod() == deribit_multicast::Period::month);
          CHECK(instrument.settlementPeriodCount() == 1);
          CHECK(instrument.getBaseCurrencyAsStringView() == "BTC"sv);
          CHECK(instrument.getQuoteCurrencyAsStringView() == "BTC"sv);
          CHECK(instrument.getCounterCurrencyAsStringView() == "USD"sv);
          CHECK(instrument.getSettlementCurrencyAsStringView() == "BTC"sv);
          CHECK(instrument.getSizeCurrencyAsStringView() == "BTC"sv);
          CHECK(instrument.creationTimestampMs() == 1722840363000);
          CHECK(instrument.expirationTimestampMs() == 1729843200000);
          CHECK(instrument.strikePrice() == Catch::Approx{47000.0});
          CHECK(instrument.contractSize() == Catch::Approx{1.0});
          CHECK(instrument.minTradeAmount() == Catch::Approx{0.1});
          CHECK(instrument.tickSize() == Catch::Approx{0.0001});
          CHECK(instrument.makerCommission() == Catch::Approx{0.0003});
          CHECK(instrument.takerCommission() == Catch::Approx{0.0003});
          CHECK(instrument.blockTradeCommission() == Catch::Approx{0.0003});
          CHECK(std::isnan(instrument.maxLiquidationCommission()));
          CHECK(std::isnan(instrument.maxLeverage()));
          size_t count = 0;
          const_cast<value_type &>(instrument).tickStepsList().forEach([&count](auto &item) {
            switch (++count) {
              case 1:
                CHECK(item.abovePrice() == Catch::Approx{0.005});
                CHECK(item.tickSize() == Catch::Approx{0.0005});
                break;
            }
          });
          CHECK(count == 1);
          CHECK(instrument.instrumentNameLength() == 19);
          CHECK(sbe::get_instrument_name(const_cast<value_type &>(instrument)) == "BTC-25OCT24-47000-P"sv);
          break;
        }
        case 2: {
          CHECK(instrument.instrumentId() == 285000);
          CHECK(instrument.instrumentState() == deribit_multicast::InstrumentState::open);
          CHECK(instrument.kind() == deribit_multicast::InstrumentKind::option);
          CHECK(instrument.instrumentType() == deribit_multicast::InstrumentType::reversed);
          CHECK(instrument.optionType() == deribit_multicast::OptionType::call);
          CHECK(instrument.settlementPeriod() == deribit_multicast::Period::month);
          CHECK(instrument.settlementPeriodCount() == 3);
          CHECK(instrument.getBaseCurrencyAsStringView() == "BTC"sv);
          CHECK(instrument.getQuoteCurrencyAsStringView() == "BTC"sv);
          CHECK(instrument.getCounterCurrencyAsStringView() == "USD"sv);
          CHECK(instrument.getSettlementCurrencyAsStringView() == "BTC"sv);
          CHECK(instrument.getSizeCurrencyAsStringView() == "BTC"sv);
          CHECK(instrument.creationTimestampMs() == 1703750409000);
          CHECK(instrument.expirationTimestampMs() == 1735286400000);
          CHECK(instrument.strikePrice() == Catch::Approx{15000.0});
          CHECK(instrument.contractSize() == Catch::Approx{1.0});
          CHECK(instrument.minTradeAmount() == Catch::Approx{0.1});
          CHECK(instrument.tickSize() == Catch::Approx{0.0001});
          CHECK(instrument.makerCommission() == Catch::Approx{0.0003});
          CHECK(instrument.takerCommission() == Catch::Approx{0.0003});
          CHECK(instrument.blockTradeCommission() == Catch::Approx{0.0003});
          CHECK(std::isnan(instrument.maxLiquidationCommission()));
          CHECK(std::isnan(instrument.maxLeverage()));
          size_t count = 0;
          const_cast<value_type &>(instrument).tickStepsList().forEach([&count](auto &item) {
            switch (++count) {
              case 1:
                CHECK(item.abovePrice() == Catch::Approx{0.005});
                CHECK(item.tickSize() == Catch::Approx{0.0005});
                break;
            }
          });
          CHECK(count == 1);
          CHECK(instrument.instrumentNameLength() == 19);
          CHECK(sbe::get_instrument_name(const_cast<value_type &>(instrument)) == "BTC-27DEC24-15000-C"sv);
          break;
        }
        case 3: {
          CHECK(instrument.instrumentId() == 354401);
          CHECK(instrument.instrumentState() == deribit_multicast::InstrumentState::open);
          CHECK(instrument.kind() == deribit_multicast::InstrumentKind::option);
          CHECK(instrument.instrumentType() == deribit_multicast::InstrumentType::reversed);
          CHECK(instrument.optionType() == deribit_multicast::OptionType::call);
          CHECK(instrument.settlementPeriod() == deribit_multicast::Period::month);
          CHECK(instrument.settlementPeriodCount() == 1);
          CHECK(instrument.getBaseCurrencyAsStringView() == "BTC"sv);
          CHECK(instrument.getQuoteCurrencyAsStringView() == "BTC"sv);
          CHECK(instrument.getCounterCurrencyAsStringView() == "USD"sv);
          CHECK(instrument.getSettlementCurrencyAsStringView() == "BTC"sv);
          CHECK(instrument.getSizeCurrencyAsStringView() == "BTC"sv);
          CHECK(instrument.creationTimestampMs() == 1722540000000);
          CHECK(instrument.expirationTimestampMs() == 1729843200000);
          CHECK(instrument.strikePrice() == Catch::Approx{57000.0});
          CHECK(instrument.contractSize() == Catch::Approx{1.0});
          CHECK(instrument.minTradeAmount() == Catch::Approx{0.1});
          CHECK(instrument.tickSize() == Catch::Approx{0.0001});
          CHECK(instrument.makerCommission() == Catch::Approx{0.0003});
          CHECK(instrument.takerCommission() == Catch::Approx{0.0003});
          CHECK(instrument.blockTradeCommission() == Catch::Approx{0.0003});
          CHECK(std::isnan(instrument.maxLiquidationCommission()));
          CHECK(std::isnan(instrument.maxLeverage()));
          size_t count = 0;
          const_cast<value_type &>(instrument).tickStepsList().forEach([&count](auto &item) {
            switch (++count) {
              case 1:
                CHECK(item.abovePrice() == Catch::Approx{0.005});
                CHECK(item.tickSize() == Catch::Approx{0.0005});
                break;
            }
          });
          CHECK(count == 1);
          CHECK(instrument.instrumentNameLength() == 19);
          CHECK(sbe::get_instrument_name(const_cast<value_type &>(instrument)) == "BTC-25OCT24-57000-C"sv);
          break;
        }
        default:
          FAIL();
      }
    }

   private:
    size_t frame_count_ = {};
    size_t instrument_count_ = {};
    size_t ticker_count_ = {};
    size_t snapshot_count_ = {};
    size_t instrument_v2_count_ = {};
  } handler;

  REQUIRE(std::size(message) == 1447);
  std::span buffer{reinterpret_cast<std::byte const *>(std::data(message)), std::size(message)};

  TraceInfo trace_info;
  auto result = sbe::Parser::dispatch(handler, buffer, trace_info);
  CHECK(result == true);
  handler.finished();
}
