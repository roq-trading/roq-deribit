/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

using value_type = json::UserOrders;

TEST_CASE("simple", "[json_user_orders]") {
  auto message = R"({)"
                 R"("jsonrpc":"2.0",)"
                 R"("method":"subscription",)"
                 R"("params":{)"
                 R"("channel":"user.orders.any.any.raw",)"
                 R"("data":{)"
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
                 R"(})"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.params.channel == "user.orders.any.any.raw"sv);
    auto &data = obj.params.data;
    CHECK(data.label == "YwACqyGDa0UAAQAAAAAA"sv);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
