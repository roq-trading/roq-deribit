/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/deribit/json/ticker.h"

using namespace roq;  // NOLINT
using namespace roq::deribit;  // NOLINT

TEST(json_ticker, parse_message) {
  const char *message =
  R"({)"
  R"("jsonrpc":"2.0",)"
  R"("result":{)"
  R"("timestamp":1566822213978,)"
  R"("stats":{)"
  R"("volume":5739.09193092,)"
  R"("low":9909.0,)"
  R"("high":10679.0)"
  R"(},)"
  R"("state":"open",)"
  R"("settlement_price":10053.92,)"
  R"("open_interest":422565281,)"
  R"("min_price":10346.94,)"
  R"("max_price":10450.93,)"
  R"("mark_price":10399.67,)"
  R"("last_price":10398.5,)"
  R"("instrument_name":"BTC-PERPETUAL",)"
  R"("index_price":10391.86,)"
  R"("funding_8h":0.00011248,)"
  R"("current_funding":0.00025155,)"
  R"("best_bid_price":10398.5,)"
  R"("best_bid_amount":219330.0,)"
  R"("best_ask_price":10399.0,)"
  R"("best_ask_amount":97030.0)"
  R"(},)"
  R"("usIn":1566822214076412,)"
  R"("usOut":1566822214076605,)"
  R"("usDiff":193,)"
  R"("testnet":true)"
  R"(})";
  int results = 0;
  core::json::Parser parser(message);
  auto root = parser.root();
  for (auto [key, value] : std::get<core::json::object_t>(root)) {
    if (key.compare("result") == 0) {
      ++results;
      json::Ticker ticker(value);
      EXPECT_EQ(ticker.state, json::State::OPEN);
      EXPECT_DOUBLE_EQ(ticker.settlement_price, 10053.92);
      EXPECT_DOUBLE_EQ(ticker.open_interest, 422565281);
      EXPECT_DOUBLE_EQ(ticker.min_price, 10346.94);
      EXPECT_DOUBLE_EQ(ticker.max_price, 10450.93);
      EXPECT_DOUBLE_EQ(ticker.mark_price, 10399.67);
      EXPECT_DOUBLE_EQ(ticker.last_price, 10398.5);
      EXPECT_EQ(ticker.instrument_name, "BTC-PERPETUAL");
      EXPECT_DOUBLE_EQ(ticker.index_price, 10391.86);
      EXPECT_DOUBLE_EQ(ticker.funding_8h, 0.00011248);
      EXPECT_DOUBLE_EQ(ticker.current_funding, 0.00025155);
      EXPECT_DOUBLE_EQ(ticker.best_bid_price, 10398.5);
      EXPECT_DOUBLE_EQ(ticker.best_bid_amount, 219330.0);
      EXPECT_DOUBLE_EQ(ticker.best_ask_price, 10399.0);
      EXPECT_DOUBLE_EQ(ticker.best_ask_amount, 97030.0);
    }
  }
  EXPECT_EQ(results, 1);
}
