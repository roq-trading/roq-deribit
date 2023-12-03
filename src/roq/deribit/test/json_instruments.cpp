/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/deribit/json/instrument.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("json_instrument_parse_message", "[json_instrument]") {
  auto const message = R"({)"
                       R"("jsonrpc":"2.0",)"
                       R"("result":[)"
                       R"({)"
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

  int results = 0, instruments = 0;
  core::json::Parser parser(message);
  auto root = parser.root();
  for (auto [key, value] : std::get<core::json::Object>(root)) {
    if (key.compare("result"sv) == 0) {
      ++results;
      for (auto iter : std::get<core::json::Array>(value)) {
        ++instruments;
        json::Instrument instrument(iter);
        switch (instruments) {
          case 1:
            CHECK(instrument.tick_size == 0.5_a);
            CHECK(instrument.taker_commission == 0.0005_a);
            CHECK(instrument.settlement_period == "month"sv);
            CHECK(instrument.quote_currency == "USD"sv);
            CHECK(instrument.min_trade_amount == 10.0_a);
            CHECK(instrument.max_leverage == 100.0_a);
            CHECK(instrument.maker_commission == -0.0002_a);
            CHECK(instrument.kind == json::Kind::FUTURE);
            CHECK(instrument.is_active == true);
            CHECK(instrument.instrument_name == "BTC-27SEP19"sv);
            CHECK(instrument.expiration_timestamp == 1569571200000ms);
            CHECK(instrument.creation_timestamp == 1553760060000ms);
            CHECK(instrument.contract_size == 10.0_a);
            CHECK(instrument.base_currency == "BTC"sv);
            break;
          case 2:
            CHECK(instrument.tick_size == 0.5_a);
            CHECK(instrument.taker_commission == 0.0005_a);
            CHECK(instrument.settlement_period == "month"sv);
            CHECK(instrument.quote_currency == "USD"sv);
            CHECK(instrument.min_trade_amount == 10.0_a);
            CHECK(instrument.max_leverage == 100.0_a);
            CHECK(instrument.maker_commission == -0.0002_a);
            CHECK(instrument.kind == json::Kind::FUTURE);
            CHECK(instrument.is_active == true);
            CHECK(instrument.instrument_name == "BTC-27DEC19"sv);
            CHECK(instrument.expiration_timestamp == 1577433600000ms);
            CHECK(instrument.creation_timestamp == 1561622460000ms);
            CHECK(instrument.contract_size == 10.0_a);
            CHECK(instrument.base_currency == "BTC"sv);
            break;
          case 3:
            CHECK(instrument.tick_size == 0.5_a);
            CHECK(instrument.taker_commission == 0.00075_a);
            CHECK(instrument.settlement_period == "perpetual"sv);
            CHECK(instrument.quote_currency == "USD"sv);
            CHECK(instrument.min_trade_amount == 10.0_a);
            CHECK(instrument.max_leverage == 100.0_a);
            CHECK(instrument.maker_commission == -0.00025_a);
            CHECK(instrument.kind == json::Kind::FUTURE);
            CHECK(instrument.is_active == true);
            CHECK(instrument.instrument_name == "BTC-PERPETUAL"sv);
            CHECK(instrument.expiration_timestamp == 32503734000000ms);
            CHECK(instrument.creation_timestamp == 1534167754000ms);
            CHECK(instrument.contract_size == 10.0_a);
            CHECK(instrument.base_currency == "BTC"sv);
            break;
        }
      }
    }
  }
  CHECK(results == 1);
  CHECK(instruments == 3);
}
