/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/deribit/json/instrument.h"

using namespace roq;           // NOLINT
using namespace roq::deribit;  // NOLINT

TEST(json_instrument, parse_message) {
  const char *message = R"({)"
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
                        R"(})";

  int results = 0, instruments = 0;
  core::json::Parser parser(message);
  auto root = parser.root();
  for (auto [key, value] : std::get<core::json::object_t>(root)) {
    if (key.compare("result") == 0) {
      ++results;
      for (auto iter : std::get<core::json::array_t>(value)) {
        ++instruments;
        json::Instrument instrument(iter);
        switch (instruments) {
          case 1:
            EXPECT_DOUBLE_EQ(instrument.tick_size, 0.5);
            EXPECT_DOUBLE_EQ(instrument.taker_commission, 0.0005);
            EXPECT_EQ(instrument.settlement_period, "month");
            EXPECT_EQ(instrument.quote_currency, "USD");
            EXPECT_DOUBLE_EQ(instrument.min_trade_amount, 10.0);
            EXPECT_DOUBLE_EQ(instrument.max_leverage, 100.0);
            EXPECT_DOUBLE_EQ(instrument.maker_commission, -0.0002);
            EXPECT_EQ(instrument.kind, json::Kind::FUTURE);
            EXPECT_EQ(instrument.is_active, true);
            EXPECT_EQ(instrument.instrument_name, "BTC-27SEP19");
            EXPECT_EQ(
                instrument.expiration_timestamp,
                std::chrono::milliseconds { 1569571200000 });
            EXPECT_EQ(
                instrument.creation_timestamp,
                std::chrono::milliseconds { 1553760060000 });
            EXPECT_DOUBLE_EQ(instrument.contract_size, 10.0);
            EXPECT_EQ(instrument.base_currency, "BTC");
            break;
          case 2:
            EXPECT_DOUBLE_EQ(instrument.tick_size, 0.5);
            EXPECT_DOUBLE_EQ(instrument.taker_commission, 0.0005);
            EXPECT_EQ(instrument.settlement_period, "month");
            EXPECT_EQ(instrument.quote_currency, "USD");
            EXPECT_DOUBLE_EQ(instrument.min_trade_amount, 10.0);
            EXPECT_DOUBLE_EQ(instrument.max_leverage, 100.0);
            EXPECT_DOUBLE_EQ(instrument.maker_commission, -0.0002);
            EXPECT_EQ(instrument.kind, json::Kind::FUTURE);
            EXPECT_EQ(instrument.is_active, true);
            EXPECT_EQ(instrument.instrument_name, "BTC-27DEC19");
            EXPECT_EQ(
                instrument.expiration_timestamp,
                std::chrono::milliseconds { 1577433600000 });
            EXPECT_EQ(
                instrument.creation_timestamp,
                std::chrono::milliseconds { 1561622460000 });
            EXPECT_DOUBLE_EQ(instrument.contract_size, 10.0);
            EXPECT_EQ(instrument.base_currency, "BTC");
            break;
          case 3:
            EXPECT_DOUBLE_EQ(instrument.tick_size, 0.5);
            EXPECT_DOUBLE_EQ(instrument.taker_commission, 0.00075);
            EXPECT_EQ(instrument.settlement_period, "perpetual");
            EXPECT_EQ(instrument.quote_currency, "USD");
            EXPECT_DOUBLE_EQ(instrument.min_trade_amount, 10.0);
            EXPECT_DOUBLE_EQ(instrument.max_leverage, 100.0);
            EXPECT_DOUBLE_EQ(instrument.maker_commission, -0.00025);
            EXPECT_EQ(instrument.kind, json::Kind::FUTURE);
            EXPECT_EQ(instrument.is_active, true);
            EXPECT_EQ(instrument.instrument_name, "BTC-PERPETUAL");
            EXPECT_EQ(
                instrument.expiration_timestamp,
                std::chrono::milliseconds { 32503734000000 });
            EXPECT_EQ(
                instrument.creation_timestamp,
                std::chrono::milliseconds { 1534167754000 });
            EXPECT_DOUBLE_EQ(instrument.contract_size, 10.0);
            EXPECT_EQ(instrument.base_currency, "BTC");
            break;
        }
      }
    }
  }
  EXPECT_EQ(results, 1);
  EXPECT_EQ(instruments, 3);
}
