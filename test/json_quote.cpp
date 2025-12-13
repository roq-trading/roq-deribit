/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

using value_type = json::Quote;

/*
TEST_CASE("subscribe_ack", "[json_quote]") {
  auto message = R"({)"
                 R"("jsonrpc":"2.0",)"
                 R"("id":"subscribe_quote",)"
                 R"("result":[)"
                 R"("quote.ETH_USDC-PERPETUAL",)"
                 R"("quote.ETH-PERPETUAL",)"
                 R"("quote.ETH-30JAN26",)"
                 R"("quote.ETH-27MAR26",)"
                 R"("quote.ETH-26JUN26",)"
                 R"("quote.ETH-26DEC25",)"
                 R"("quote.ETH-25SEP26",)"
                 R"("quote.ETH-19DEC25",)"
                 R"("quote.ETH-12DEC25",)"
                 R"("quote.BTC_USDC-PERPETUAL",)"
                 R"("quote.BTC-PERPETUAL",)"
                 R"("quote.BTC-30JAN26",)"
                 R"("quote.BTC-27MAR26",)"
                 R"("quote.BTC-26JUN26",)"
                 R"("quote.BTC-26DEC25",)"
                 R"("quote.BTC-25SEP26",)"
                 R"("quote.BTC-19DEC25",)"
                 R"("quote.BTC-12DEC25")"
                 R"(],)"
                 R"("usIn":1765425743121219,)"
                 R"("usOut":1765425743122229,)"
                 R"("usDiff":1010,)"
                 R"("testnet":false)"
                 R"(})"sv;
}
*/
TEST_CASE("simple", "[json_quote]") {
  auto message = R"({)"
                 R"("jsonrpc":"2.0",)"
                 R"("method":"subscription",)"
                 R"("params":{)"
                 R"("channel":"quote.ETH_USDC-PERPETUAL",)"
                 R"("data":{)"
                 R"("timestamp":1765425743087,)"
                 R"("instrument_name":"ETH_USDC-PERPETUAL",)"
                 R"("best_ask_price":3187.25,)"
                 R"("best_bid_price":3187.2,)"
                 R"("best_ask_amount":0.133,)"
                 R"("best_bid_amount":0.069)"
                 R"(})"
                 R"(})"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.params.channel == "quote.ETH_USDC-PERPETUAL"sv);
    auto &data = obj.params.data;
    CHECK(data.timestamp == 1765425743087ms);
    CHECK(data.instrument_name == "ETH_USDC-PERPETUAL"sv);
    CHECK(data.best_ask_price == 3187.25_a);
    CHECK(data.best_bid_price == 3187.20_a);
    CHECK(data.best_ask_amount == 0.133_a);
    CHECK(data.best_bid_amount == 0.069_a);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
