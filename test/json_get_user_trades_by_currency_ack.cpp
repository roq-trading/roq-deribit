/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

using value_type = json::GetUserTradesByCurrencyAck;

TEST_CASE("failure", "[json_get_user_trades_by_currency_ack]") {
  auto message = R"({)"
                 R"("jsonrpc":"2.0",)"
                 R"("id":"get_user_trades_by_currency",)"
                 R"("error":{)"
                 R"("code":-32602,)"
                 R"("data":{)"
                 R"("reason":"value required",)"
                 R"("param":"currency")"
                 R"(},)"
                 R"("message":"Invalid params")"
                 R"(},)"
                 R"("usIn":1765525372076064,)"
                 R"("usOut":1765525372076107,)"
                 R"("usDiff":43,)"
                 R"("testnet":true)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.id == "get_user_trades_by_currency"sv);
    CHECK(obj.error.code == -32602);
    CHECK(obj.error.message == "Invalid params"sv);
    CHECK(obj.error.data.reason == "value required"sv);
    CHECK(obj.error.data.param == "currency"sv);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

TEST_CASE("empty", "[json_get_user_trades_by_currency_ack]") {
  auto message = R"({)"
                 R"("jsonrpc":"2.0",)"
                 R"("id":"get_user_trades_by_currency",)"
                 R"("result":{)"
                 R"("trades":[],)"
                 R"("has_more":false)"
                 R"(},)"
                 R"("usIn":1765463128400705,)"
                 R"("usOut":1765463128429088,)"
                 R"("usDiff":28383,)"
                 R"("testnet":false)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.id == "get_user_trades_by_currency"sv);
    CHECK(obj.error.code == 0);
    CHECK(std::empty(obj.result.trades));
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

TEST_CASE("success", "[json_get_user_trades_by_currency_ack]") {
  auto message = R"({)"
                 R"("jsonrpc":"2.0",)"
                 R"("id":"get_user_trades_by_currency",)"
                 R"("result":{)"
                 R"("trades":[{)"
                 R"("label":"fwAC2hbYakUAAQAAAAAA",)"
                 R"("timestamp":1765504234533,)"
                 R"("state":"filled",)"
                 R"("price":91701.5,)"
                 R"("user_id":10580,)"
                 R"("amount":10.0,)"
                 R"("direction":"sell",)"
                 R"("index_price":92158.38,)"
                 R"("instrument_name":"BTC-PERPETUAL",)"
                 R"("trade_seq":123024280,)"
                 R"("api":false,)"
                 R"("mark_price":92149.91,)"
                 R"("order_id":"74364388208",)"
                 R"("matching_id":null,)"
                 R"("tick_direction":2,)"
                 R"("profit_loss":0.0,)"
                 R"("post_only":false,)"
                 R"("reduce_only":false,)"
                 R"("self_trade":false,)"
                 R"("mmp":false,)"
                 R"("contracts":1.0,)"
                 R"("trade_id":"208465071",)"
                 R"("fee_currency":"BTC",)"
                 R"("fee":6.0e-8,)"
                 R"("order_type":"limit",)"
                 R"("risk_reducing":false,)"
                 R"("liquidity":"T")"
                 R"(})"
                 R"(],)"
                 R"("has_more":false)"
                 R"(},)"
                 R"("usIn":1765504625940284,)"
                 R"("usOut":1765504625971050,)"
                 R"("usDiff":30766,)"
                 R"("testnet":true)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.id == "get_user_trades_by_currency"sv);
    CHECK(obj.error.code == 0);
    REQUIRE(std::size(obj.result.trades) == 1);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
