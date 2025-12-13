/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

using value_type = json::UserTrades;

TEST_CASE("simple", "[json_user_trades]") {
  auto message = R"({)"
                 R"("jsonrpc":"2.0",)"
                 R"("method":"subscription",)"
                 R"("params":{)"
                 R"("channel":"user.trades.any.any.raw",)"
                 R"("data":[{)"
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
                 R"(])"
                 R"(})"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.params.channel == "user.trades.any.any.raw"sv);
    auto &data = obj.params.data;
    CHECK(data[0].label == "YwACqyGDa0UAAQAAAAAA"sv);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
