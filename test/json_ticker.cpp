/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <catch2/catch.hpp>

#include "roq/deribit/json/ticker.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

TEST_CASE("json_ticker_parse_message", "[json_ticker]") {
  const auto message = R"({)"
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
                       R"(})"sv;
  int results = 0;
  core::json::Parser parser(message);
  auto root = parser.root();
  for (auto [key, value] : std::get<core::json::Object>(root)) {
    if (key.compare("result"sv) == 0) {
      ++results;
      json::Ticker ticker(value);
      CHECK(ticker.state == json::State::OPEN);
      CHECK(ticker.settlement_price == 10053.92_a);
      CHECK(ticker.open_interest == 422565281_a);
      CHECK(ticker.min_price == 10346.94_a);
      CHECK(ticker.max_price == 10450.93_a);
      CHECK(ticker.mark_price == 10399.67_a);
      CHECK(ticker.last_price == 10398.5_a);
      CHECK(ticker.instrument_name == "BTC-PERPETUAL"sv);
      CHECK(ticker.index_price == 10391.86_a);
      CHECK(ticker.funding_8h == 0.00011248_a);
      CHECK(ticker.current_funding == 0.00025155_a);
      CHECK(ticker.best_bid_price == 10398.5_a);
      CHECK(ticker.best_bid_amount == 219330.0_a);
      CHECK(ticker.best_ask_price == 10399.0_a);
      CHECK(ticker.best_ask_amount == 97030.0_a);
    }
  }
  CHECK(results == 1);
}

/*
// last_price="undefined" ???
{
"jsonrpc":"2.0",
"method":"subscription",
"params":{
"channel":"ticker.SOL-29APR22-95-C.100ms",
"data":{
"underlying_price":101.6413,
"underlying_index":"SOL-29APR22",
"timestamp":1650009559391,
"stats":{
"volume":null,
"price_change":null,
"low":null,
"high":null
},
"state":"open",
"settlement_price":0.14180605,
"open_interest":0.0,
"min_price":0.052,
"max_price":0.186,
"mark_price":0.1126,
"mark_iv":100.0,
"last_price":"undefined",
"interest_rate":0.0,
"instrument_name":"SOL-29APR22-95-C",
"index_price":101.6419,
"greeks":{
"vega":0.072,
"theta":-0.25711,
"rho":0.02178,
"gamma":0.01817,
"delta":0.6711
},
"estimated_delivery_price":101.6419,
"bid_iv":0.0,
"best_bid_price":0.0,
"best_bid_amount":0.0,
"best_ask_price":0.0,
"best_ask_amount":0.0,
"ask_iv":0.0
}
}
}
*/
