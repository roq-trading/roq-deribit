/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/deribit/json/get_currencies_ack.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

using value_type = json::GetCurrenciesAck;

TEST_CASE("simple", "[json_get_currencies_ack]") {
  auto message = R"({)"
                 R"("jsonrpc":"2.0",)"
                 R"("result":[{)"
                 R"("withdrawal_priorities":[],)"
                 R"("withdrawal_fee":0.0004,)"
                 R"("min_withdrawal_fee":0.0001,)"
                 R"("min_confirmations":4,)"
                 R"("fee_precision":4,)"
                 R"("currency_long":"Ethereum",)"
                 R"("currency":"ETH",)"
                 R"("coin_type":"ETHER")"
                 R"(},{)"
                 R"("withdrawal_priorities":[{"value":0.15,"name":"very_low"},{"value":1.5,"name":"very_high"}],)"
                 R"("withdrawal_fee":0.0001,)"
                 R"("min_withdrawal_fee":0.0001,)"
                 R"("min_confirmations":1,)"
                 R"("fee_precision":4,)"
                 R"("currency_long":"Bitcoin",)"
                 R"("currency":"BTC",)"
                 R"("coin_type":"BITCOIN")"
                 R"(})"
                 R"(],)"
                 R"("usIn":1566823367410171,)"
                 R"("usOut":1566823367410971,)"
                 R"("usDiff":800,)"
                 R"("testnet":true)"
                 R"(})"sv;
  auto helper = [&](value_type &obj) {
    REQUIRE(std::size(obj.result) == 2);
    auto &r0 = obj.result[0];
    CHECK(r0.withdrawal_fee == 0.0004_a);
    CHECK(r0.min_withdrawal_fee == 0.0001_a);
    CHECK(r0.min_confirmations == uint32_t{4});
    CHECK(r0.fee_precision == uint32_t{4});
    CHECK(r0.currency_long == "Ethereum"sv);
    CHECK(r0.currency == "ETH"sv);
    CHECK(r0.coin_type == "ETHER"sv);
    auto &r1 = obj.result[1];
    CHECK(r1.withdrawal_fee == 0.0001_a);
    CHECK(r1.min_withdrawal_fee == 0.0001_a);
    CHECK(r1.min_confirmations == uint32_t{1});
    CHECK(r1.fee_precision == uint32_t{4});
    CHECK(r1.currency_long == "Bitcoin"sv);
    CHECK(r1.currency == "BTC"sv);
    CHECK(r1.coin_type == "BITCOIN"sv);
  };
  core::json::BufferStack buffers{8192, 2};
  value_type obj{message, buffers};
  helper(obj);
}
