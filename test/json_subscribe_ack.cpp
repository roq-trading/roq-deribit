/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

using value_type = json::SubscribeAck;

TEST_CASE("failure", "[json_subscribe_ack]") {
  auto message = R"({)"
                 R"("jsonrpc":"2.0",)"
                 R"("id":"subscribe_user_portfolio",)"
                 R"("error":{)"
                 R"("code":-32601,)"
                 R"("message":"Method not found")"
                 R"(},)"
                 R"("usIn":1765524574265551,)"
                 R"("usOut":1765524574265673,)"
                 R"("usDiff":122,)"
                 R"("testnet":true)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.id == "subscribe_user_portfolio"sv);
    CHECK(obj.error.code == -32601);
    CHECK(obj.error.message == "Method not found"sv);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

TEST_CASE("platform_state", "[json_subscribe_ack]") {
  auto message = R"({)"
                 R"("jsonrpc":"2.0",)"
                 R"("id":"subscribe_platform_state",)"
                 R"("result":[)"
                 R"("platform_state")"
                 R"(],)"
                 R"("usIn":1765525372073376,)"
                 R"("usOut":1765525372073529,)"
                 R"("usDiff":153,)"
                 R"("testnet":true)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.id == "subscribe_platform_state"sv);
    CHECK(obj.error.code == 0);
    REQUIRE(std::size(obj.result) == 1);
    CHECK(obj.result[0] == "platform_state"sv);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

TEST_CASE("quote", "[json_subscribe_ack]") {
  auto message = R"({)"
                 R"("jsonrpc":"2.0",)"
                 R"("id":"subscribe_quote",)"
                 R"("result":[)"
                 R"("quote.ETH_USDT",)"
                 R"("quote.ETH_USDE",)"
                 R"("quote.ETH_USDC",)"
                 R"("quote.ETH-PERPETUAL",)"
                 R"("quote.ETH-30JAN26",)"
                 R"("quote.ETH-27MAR26",)"
                 R"("quote.ETH-26JUN26",)"
                 R"("quote.ETH-26DEC25",)"
                 R"("quote.ETH-25SEP26",)"
                 R"("quote.ETH-19DEC25",)"
                 R"("quote.ETH-12DEC25",)"
                 R"("quote.BTC_USDT",)"
                 R"("quote.BTC_USDE",)"
                 R"("quote.BTC_USDC-PERPETUAL",)"
                 R"("quote.BTC_USDC",)"
                 R"("quote.BTC-PERPETUAL",)"
                 R"("quote.BTC-30JAN26",)"
                 R"("quote.BTC-27MAR26",)"
                 R"("quote.BTC-27FEB26",)"
                 R"("quote.BTC-26JUN26",)"
                 R"("quote.BTC-26DEC25",)"
                 R"("quote.BTC-25SEP26",)"
                 R"("quote.BTC-19DEC25",)"
                 R"("quote.BTC-12DEC25")"
                 R"(],)"
                 R"("usIn":1765525372073736,)"
                 R"("usOut":1765525372073961,)"
                 R"("usDiff":225,)"
                 R"("testnet":true)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.id == "subscribe_quote"sv);
    CHECK(obj.error.code == 0);
    REQUIRE(std::size(obj.result) == 24);
    CHECK(obj.result[0] == "quote.ETH_USDT"sv);
    CHECK(obj.result[23] == "quote.BTC-12DEC25"sv);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
