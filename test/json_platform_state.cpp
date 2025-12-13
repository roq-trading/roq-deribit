/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

using value_type = json::PlatformState;
/*
TEST_CASE("subscribe_ack", "[json_platform_state]") {
  auto message = R"({)"
                 R"("jsonrpc":"2.0",)"
                 R"("id":"subscribe_platform_state",)"
                 R"("result":[)"
                 R"("platform_state")"
                 R"(],)"
                 R"("usIn":1765425743120719,)"
                 R"("usOut":1765425743120957,)"
                 R"("usDiff":238,)"
                 R"("testnet":false)"
                 R"(})"sv;
}
*/
TEST_CASE("simple", "[json_platform_state]") {
  auto message = R"({)"
                 R"("jsonrpc":"2.0",)"
                 R"("method":"subscription",)"
                 R"("params":{)"
                 R"("channel":"platform_state",)"
                 R"("data":{)"
                 R"("locked":false,)"
                 R"("price_index":"eth_usdc")"
                 R"(})"
                 R"(})"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.params.channel == "platform_state"sv);
    auto &data = obj.params.data;
    CHECK(data.locked == false);
    CHECK(data.price_index == "eth_usdc"sv);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}
