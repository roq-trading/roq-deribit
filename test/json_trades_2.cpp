/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/deribit/json/trades_2.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

TEST_CASE("json_trades_2_parse_message", "[json_trades_2]") {
  auto const message = R"([{)"
                       R"("trade_seq":52491427,)"
                       R"("trade_id":"76203357",)"
                       R"("timestamp":1625752501753,)"
                       R"("tick_direction":2,)"
                       R"("state":"filled",)"
                       R"("self_trade":false,)"
                       R"("reduce_only":false,)"
                       R"("profit_loss":0.0,)"
                       R"("price":32477.0,)"
                       R"("post_only":false,)"
                       R"("order_type":"limit",)"
                       R"("order_id":"6089547059",)"
                       R"("matching_id":null,)"
                       R"("mark_price":32472.43,)"
                       R"("liquidity":"M",)"
                       R"("label":"roq-2-1001",)"
                       R"("instrument_name":"BTC-PERPETUAL",)"
                       R"("index_price":32477.38,)"
                       R"("fee_currency":"BTC",)"
                       R"("fee":0.0,)"
                       R"("direction":"buy",)"
                       R"("amount":10.0)"
                       R"(}])"sv;
  core::Buffer buffer(8192);
  core::json::Buffer decode_buffer(buffer);
  auto trades = core::json::Parser::create<json::Trades2>(message, decode_buffer);
  CHECK(std::size(trades.data) == 1);
  CHECK(trades.data[0].trade_seq == 52491427);
}
