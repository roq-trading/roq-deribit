/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/deribit/json/get_instruments_ack.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using value_type = json::GetInstrumentsAck;

TEST_CASE("simple", "[json_get_instruments_ack]") {
  auto message = R"({)"
                 R"("jsonrpc":"2.0",)"
                 R"("result":[{)"
                 R"("tick_size":0.5,)"
                 R"("taker_commission":0.0005,)"
                 R"("settlement_period":"month",)"
                 R"("quote_currency":"USD",)"
                 R"("min_trade_amount":10.0,)"
                 R"("max_leverage":100,)"
                 R"("maker_commission":-0.0002,)"
                 R"("kind":"future",)"
                 R"("is_active":true,)"
                 R"("instrument_name":"BTC-27SEP19",)"
                 R"("expiration_timestamp":1569571200000,)"
                 R"("creation_timestamp":1553760060000,)"
                 R"("contract_size":10.0,)"
                 R"("base_currency":"BTC")"
                 R"(},{)"
                 R"("tick_size":0.5,)"
                 R"("taker_commission":0.0005,)"
                 R"("settlement_period":"month",)"
                 R"("quote_currency":"USD",)"
                 R"("min_trade_amount":10.0,)"
                 R"("max_leverage":100,)"
                 R"("maker_commission":-0.0002,)"
                 R"("kind":"future",)"
                 R"("is_active":true,)"
                 R"("instrument_name":"BTC-27DEC19",)"
                 R"("expiration_timestamp":1577433600000,)"
                 R"("creation_timestamp":1561622460000,)"
                 R"("contract_size":10.0,)"
                 R"("base_currency":"BTC")"
                 R"(},{)"
                 R"("tick_size":0.5,)"
                 R"("taker_commission":0.00075,)"
                 R"("settlement_period":"perpetual",)"
                 R"("quote_currency":"USD",)"
                 R"("min_trade_amount":10.0,)"
                 R"("max_leverage":100,)"
                 R"("maker_commission":-0.00025,)"
                 R"("kind":"future",)"
                 R"("is_active":true,)"
                 R"("instrument_name":"BTC-PERPETUAL",)"
                 R"("expiration_timestamp":32503734000000,)"
                 R"("creation_timestamp":1534167754000,)"
                 R"("contract_size":10.0,)"
                 R"("base_currency":"BTC")"
                 R"(})"
                 R"(],)"
                 R"("usIn":1566829640857411,)"
                 R"("usOut":1566829640859601,)"
                 R"("usDiff":2190,)"
                 R"("testnet":true)"
                 R"(})"sv;
  auto helper = [&](value_type &obj) {
    REQUIRE(std::size(obj.result) == 3);
    auto &r0 = obj.result[0];
    CHECK(r0.tick_size == 0.5_a);
    CHECK(r0.taker_commission == 0.0005_a);
    CHECK(r0.settlement_period == json::SettlementPeriod::MONTH);
    CHECK(r0.quote_currency == "USD"sv);
    CHECK(r0.min_trade_amount == 10.0_a);
    CHECK(r0.max_leverage == 100.0_a);
    CHECK(r0.maker_commission == -0.0002_a);
    CHECK(r0.kind == json::Kind::FUTURE);
    CHECK(r0.is_active == true);
    CHECK(r0.instrument_name == "BTC-27SEP19"sv);
    CHECK(r0.expiration_timestamp == 1569571200000ms);
    CHECK(r0.creation_timestamp == 1553760060000ms);
    CHECK(r0.contract_size == 10.0_a);
    CHECK(r0.base_currency == "BTC"sv);
    auto &r1 = obj.result[1];
    CHECK(r1.tick_size == 0.5_a);
    CHECK(r1.taker_commission == 0.0005_a);
    CHECK(r1.settlement_period == json::SettlementPeriod::MONTH);
    CHECK(r1.quote_currency == "USD"sv);
    CHECK(r1.min_trade_amount == 10.0_a);
    CHECK(r1.max_leverage == 100.0_a);
    CHECK(r1.maker_commission == -0.0002_a);
    CHECK(r1.kind == json::Kind::FUTURE);
    CHECK(r1.is_active == true);
    CHECK(r1.instrument_name == "BTC-27DEC19"sv);
    CHECK(r1.expiration_timestamp == 1577433600000ms);
    CHECK(r1.creation_timestamp == 1561622460000ms);
    CHECK(r1.contract_size == 10.0_a);
    CHECK(r1.base_currency == "BTC"sv);
    auto &r2 = obj.result[2];
    CHECK(r2.tick_size == 0.5_a);
    CHECK(r2.taker_commission == 0.00075_a);
    CHECK(r2.settlement_period == json::SettlementPeriod::PERPETUAL);
    CHECK(r2.quote_currency == "USD"sv);
    CHECK(r2.min_trade_amount == 10.0_a);
    CHECK(r2.max_leverage == 100.0_a);
    CHECK(r2.maker_commission == -0.00025_a);
    CHECK(r2.kind == json::Kind::FUTURE);
    CHECK(r2.is_active == true);
    CHECK(r2.instrument_name == "BTC-PERPETUAL"sv);
    CHECK(r2.expiration_timestamp == 32503734000000ms);
    CHECK(r2.creation_timestamp == 1534167754000ms);
    CHECK(r2.contract_size == 10.0_a);
    CHECK(r2.base_currency == "BTC"sv);
  };
  core::json::BufferStack buffers{8192, 1};
  value_type obj{message, buffers};
  helper(obj);
}
