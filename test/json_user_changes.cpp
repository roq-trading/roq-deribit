/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

using value_type = json::UserChanges;

TEST_CASE("order_open", "[json_user_changes]") {
  auto message = R"({)"
                 R"("jsonrpc":"2.0",)"
                 R"("method":"subscription",)"
                 R"("params":{)"
                 R"("channel":"user.changes.any.any.raw",)"
                 R"("data":{)"
                 R"("orders":[{)"
                 R"("label":"9gACGAK_UkUAAQAAAAAA",)"
                 R"("price":3.2e4,)"
                 R"("user_id":10580,)"
                 R"("amount":10.0,)"
                 R"("direction":"buy",)"
                 R"("time_in_force":"good_til_cancelled",)"
                 R"("instrument_name":"BTC-PERPETUAL",)"
                 R"("web":false,)"
                 R"("api":false,)"
                 R"("order_id":"74311538594",)"
                 R"("creation_timestamp":1765463719142,)"
                 R"("replaced":false,)"
                 R"("filled_amount":0.0,)"
                 R"("last_update_timestamp":1765463719142,)"
                 R"("post_only":false,)"
                 R"("reduce_only":false,)"
                 R"("average_price":0.0,)"
                 R"("mmp":false,)"
                 R"("contracts":1.0,)"
                 R"("order_state":"open",)"
                 R"("order_type":"limit",)"
                 R"("is_liquidation":false,)"
                 R"("risk_reducing":false)"
                 R"(})"
                 R"(],)"
                 R"("instrument_name":"BTC-PERPETUAL",)"
                 R"("trades":[],)"
                 R"("positions":[{)"
                 R"("size":0.0,)"
                 R"("kind":"future",)"
                 R"("maintenance_margin":0.0,)"
                 R"("initial_margin":0.0,)"
                 R"("direction":"zero",)"
                 R"("index_price":90143.99,)"
                 R"("instrument_name":"BTC-PERPETUAL",)"
                 R"("settlement_price":90484.97,)"
                 R"("mark_price":90222.58,)"
                 R"("interest_value":0.19497798626254922,)"
                 R"("delta":0.0,)"
                 R"("average_price":0.0,)"
                 R"("floating_profit_loss":0.0,)"
                 R"("realized_profit_loss":0.0,)"
                 R"("total_profit_loss":0.0,)"
                 R"("realized_funding":0.0,)"
                 R"("leverage":50,)"
                 R"("size_currency":0.0)"
                 R"(})"
                 R"(])"
                 R"(})"
                 R"(})"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.params.channel == "user.changes.any.any.raw"sv);
    auto &data = obj.params.data;
    REQUIRE(std::size(data.orders) == 1);
    CHECK(data.instrument_name == "BTC-PERPETUAL"sv);
    CHECK(std::empty(data.trades));
    REQUIRE(std::size(data.positions) == 1);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

TEST_CASE("order_cancelled", "[json_user_changes]") {
  auto message = R"({)"
                 R"("jsonrpc":"2.0",)"
                 R"("method":"subscription",)"
                 R"("params":{)"
                 R"("channel":"user.changes.any.any.raw",)"
                 R"("data":{)"
                 R"("orders":[{)"
                 R"("label":"9gACGAK_UkUAAQAAAAAA",)"
                 R"("price":3.2e4,)"
                 R"("user_id":10580,)"
                 R"("amount":10.0,)"
                 R"("direction":"buy",)"
                 R"("time_in_force":"good_til_cancelled",)"
                 R"("instrument_name":"BTC-PERPETUAL",)"
                 R"("web":false,)"
                 R"("api":false,)"
                 R"("order_id":"74311538594",)"
                 R"("creation_timestamp":1765463719142,)"
                 R"("replaced":false,)"
                 R"("filled_amount":0.0,)"
                 R"("last_update_timestamp":1765464142326,)"
                 R"("post_only":false,)"
                 R"("reduce_only":false,)"
                 R"("cancel_reason":"user_request",)"
                 R"("average_price":0.0,)"
                 R"("mmp":false,)"
                 R"("contracts":1.0,)"
                 R"("order_state":"cancelled",)"
                 R"("order_type":"limit",)"
                 R"("is_liquidation":false,)"
                 R"("risk_reducing":false)"
                 R"(})"
                 R"(],)"
                 R"("instrument_name":"BTC-PERPETUAL",)"
                 R"("trades":[],)"
                 R"("positions":[{)"
                 R"("size":0.0,)"
                 R"("kind":"future",)"
                 R"("maintenance_margin":0.0,)"
                 R"("initial_margin":0.0,)"
                 R"("direction":"zero",)"
                 R"("index_price":90377.3,)"
                 R"("instrument_name":"BTC-PERPETUAL",)"
                 R"("settlement_price":90484.97,)"
                 R"("mark_price":90479.55,)"
                 R"("interest_value":0.19614952725118917,)"
                 R"("delta":0.0,)"
                 R"("average_price":0.0,)"
                 R"("floating_profit_loss":0.0,)"
                 R"("realized_profit_loss":0.0,)"
                 R"("total_profit_loss":0.0,)"
                 R"("realized_funding":0.0,)"
                 R"("leverage":50,)"
                 R"("size_currency":0.0)"
                 R"(})"
                 R"(])"
                 R"(})"
                 R"(})"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.params.channel == "user.changes.any.any.raw"sv);
    auto &data = obj.params.data;
    REQUIRE(std::size(data.orders) == 1);
    CHECK(data.instrument_name == "BTC-PERPETUAL"sv);
    CHECK(std::empty(data.trades));
    REQUIRE(std::size(data.positions) == 1);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

TEST_CASE("order_ioc_filled", "[json_user_changes]") {
  auto message = R"({)"
                 R"("jsonrpc":"2.0",)"
                 R"("method":"subscription",)"
                 R"("params":{)"
                 R"("channel":"user.changes.any.any.raw",)"
                 R"("data":{)"
                 R"("orders":[{)"
                 R"("label":"YwACqyGDa0UAAQAAAAAA",)"
                 R"("price":9.21e4,)"
                 R"("user_id":10580,)"
                 R"("amount":10.0,)"
                 R"("direction":"buy",)"
                 R"("time_in_force":"immediate_or_cancel",)"
                 R"("instrument_name":"BTC-PERPETUAL",)"
                 R"("web":false,)"
                 R"("api":false,)"
                 R"("order_id":"74365704227",)"
                 R"("creation_timestamp":1765505266849,)"
                 R"("replaced":false,)"
                 R"("filled_amount":10.0,)"
                 R"("last_update_timestamp":1765505266849,)"
                 R"("post_only":false,)"
                 R"("reduce_only":false,)"
                 R"("average_price":92037.0,)"
                 R"("mmp":false,)"
                 R"("contracts":1.0,)"
                 R"("order_state":"filled",)"
                 R"("order_type":"limit",)"
                 R"("is_liquidation":false,)"
                 R"("risk_reducing":false)"
                 R"(})"
                 R"(],)"
                 R"("instrument_name":"BTC-PERPETUAL",)"
                 R"("trades":[{)"
                 R"("label":"YwACqyGDa0UAAQAAAAAA",)"
                 R"("timestamp":1765505266849,)"
                 R"("state":"filled",)"
                 R"("price":92037.0,)"
                 R"("user_id":10580,)"
                 R"("amount":10.0,)"
                 R"("direction":"buy",)"
                 R"("index_price":92035.22,)"
                 R"("instrument_name":"BTC-PERPETUAL",)"
                 R"("trade_seq":123024306,)"
                 R"("api":false,)"
                 R"("mark_price":92035.41,)"
                 R"("order_id":"74365704227",)"
                 R"("matching_id":null,)"
                 R"("tick_direction":0,)"
                 R"("profit_loss":-4.0e-7,)"
                 R"("post_only":false,)"
                 R"("reduce_only":false,)"
                 R"("self_trade":false,)"
                 R"("mmp":false,)"
                 R"("contracts":1.0,)"
                 R"("trade_id":"208465104",)"
                 R"("fee_currency":"BTC",)"
                 R"("fee":5.0e-8,)"
                 R"("order_type":"limit",)"
                 R"("risk_reducing":false,)"
                 R"("liquidity":"T")"
                 R"(})"
                 R"(],)"
                 R"("positions":[{)"
                 R"("size":0.0,)"
                 R"("kind":"future",)"
                 R"("maintenance_margin":0.0,)"
                 R"("initial_margin":0.0,)"
                 R"("direction":"zero",)"
                 R"("index_price":92035.22,)"
                 R"("instrument_name":"BTC-PERPETUAL",)"
                 R"("settlement_price":90484.97,)"
                 R"("mark_price":92035.41,)"
                 R"("interest_value":0.2738695181418799,)"
                 R"("delta":0.0,)"
                 R"("average_price":0.0,)"
                 R"("floating_profit_loss":0.0,)"
                 R"("realized_profit_loss":-3.97e-7,)"
                 R"("total_profit_loss":0.0,)"
                 R"("realized_funding":0.0,)"
                 R"("leverage":50,)"
                 R"("size_currency":0.0)"
                 R"(})"
                 R"(])"
                 R"(})"
                 R"(})"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.params.channel == "user.changes.any.any.raw"sv);
    auto &data = obj.params.data;
    REQUIRE(std::size(data.orders) == 1);
    CHECK(data.instrument_name == "BTC-PERPETUAL"sv);
    REQUIRE(std::size(data.trades) == 1);
    REQUIRE(std::size(data.positions) == 1);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
