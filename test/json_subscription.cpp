/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/deribit/json/parser.h"

using namespace roq;
using namespace roq::deribit;

struct MyHandler : public json::Parser::Handler {
  void operator()(const server::Trace<json::PlatformState> &) {}
  void operator()(const server::Trace<json::InstrumentState> &) {}
  void operator()(const server::Trace<json::Quote> &) {}
  void operator()(const server::Trace<json::Ticker> &) {}
  void operator()(const server::Trace<json::Portfolio> &) {}
  void operator()(const server::Trace<json::Changes> &) {}
  void operator()(const server::Trace<json::Order> &) {}
  void operator()(const server::Trace<json::Trades2> &) {}
};

TEST(json_subscription, parse_message) {
  /*
  std::string xxx =
  R"({"jsonrpc":"2.0","method":"subscription","params":{"channel":"ticker.BTC-2APR20-7125-P.raw","data":{"underlying_price":6663.43,"underlying_index":"SYN.BTC-2APR20","timestamp":1585814403026,"stats":{"volume":25.6,"price_change":5.6338,"low":0.071,"high":0.075},"state":"open","settlement_price":0.12,"open_interest":25.6,"min_price":0.0385,"max_price":0.102,"mark_price":0.06926904,"mark_iv":250.0,"last_price":0.075,"interest_rate":0.0,"instrument_name":"BTC-2APR20-7125-P","index_price":6662.29,"greeks":{"vega":0.0,"theta":0.0,"rho":0.0,"gamma":0.0,"delta":0.0},"estimated_delivery_price":"expired","bid_iv":0.0,"best_bid_price":0.0005,"best_bid_amount":1.2,"best_ask_price":0.0775,"best_ask_amount":3.0,"ask_iv":500.0}}})";
  std::string_view message =
    R"({)"
    R"("jsonrpc":"2.0",)"
    R"("method":"subscription",)"
    R"("params":{)"
    R"("channel":"ticker.BTC-2APR20-7125-P.raw",)"
    R"("data":{)"
    R"("underlying_price":6663.43,)"
    R"("underlying_index":"SYN.BTC-2APR20",)"
    R"("timestamp":1585814403026,)"
    R"("stats":{)"
    R"("volume":25.6,)"
    R"("price_change":5.6338,)"
    R"("low":0.071,)"
    R"("high":0.075)"
    R"(},)"
    R"("state":"open",)"
    R"("settlement_price":0.12,)"
    R"("open_interest":25.6,)"
    R"("min_price":0.0385,)"
    R"("max_price":0.102,)"
    R"("mark_price":0.06926904,)"
    R"("mark_iv":250.0,)"
    R"("last_price":0.075,)"
    R"("interest_rate":0.0,)"
    R"("instrument_name":"BTC-2APR20-7125-P",)"
    R"("index_price":6662.29,)"
    R"("greeks":{)"
    R"("vega":0.0,)"
    R"("theta":0.0,)"
    R"("rho":0.0,)"
    R"("gamma":0.0,)"
    R"("delta":0.0)"
    R"(},)"
    R"("estimated_delivery_price":"expired",)"
    R"("bid_iv":0.0,)"
    R"("best_bid_price":0.0005,)"
    R"("best_bid_amount":1.2,)"
    R"("best_ask_price":0.0775,)"
    R"("best_ask_amount":3.0,)"
    R"("ask_iv":500.0)"
    R"(})"
    R"(})"
    R"(})";
  */
  const auto message = R"({)"
                       R"("channel":"ticker.BTC-2APR20-7125-P.raw",)"
                       R"("data":{)"
                       R"("underlying_price":6663.43,)"
                       R"("underlying_index":"SYN.BTC-2APR20",)"
                       R"("timestamp":1585814403026,)"
                       R"("stats":{)"
                       R"("volume":25.6,)"
                       R"("price_change":5.6338,)"
                       R"("low":0.071,)"
                       R"("high":0.075)"
                       R"(},)"
                       R"("state":"open",)"
                       R"("settlement_price":0.12,)"
                       R"("open_interest":25.6,)"
                       R"("min_price":0.0385,)"
                       R"("max_price":0.102,)"
                       R"("mark_price":0.06926904,)"
                       R"("mark_iv":250.0,)"
                       R"("last_price":0.075,)"
                       R"("interest_rate":0.0,)"
                       R"("instrument_name":"BTC-2APR20-7125-P",)"
                       R"("index_price":6662.29,)"
                       R"("greeks":{)"
                       R"("vega":0.0,)"
                       R"("theta":0.0,)"
                       R"("rho":0.0,)"
                       R"("gamma":0.0,)"
                       R"("delta":0.0)"
                       R"(},)"
                       R"("estimated_delivery_price":"expired",)"
                       R"("bid_iv":0.0,)"
                       R"("best_bid_price":0.0005,)"
                       R"("best_bid_amount":1.2,)"
                       R"("best_ask_price":0.0775,)"
                       R"("best_ask_amount":3.0,)"
                       R"("ask_iv":500.0)"
                       R"(})"
                       R"(})"_sv;
  core::Buffer buffer(4096);
  core::json::Buffer buffer_(buffer);
  core::json::Parser parser(message);
  auto root = parser.root();
  MyHandler handler;
  server::TraceInfo trace_info;
  json::Parser::dispatch(handler, root, buffer_, trace_info);
}
