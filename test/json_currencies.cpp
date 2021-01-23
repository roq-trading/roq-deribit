/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/deribit/json/currency.h"

using namespace roq;           // NOLINT
using namespace roq::deribit;  // NOLINT

TEST(json_currency, parse_message) {
  const char *message =
      R"({)"
      R"("jsonrpc":"2.0",)"
      R"("result":[)"
      R"({)"
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
      R"(})";

  int results = 0, currencies = 0;
  core::json::Parser parser(message);
  auto root = parser.root();
  for (auto [key, value] : std::get<core::json::object_t>(root)) {
    if (key.compare("result") == 0) {
      ++results;
      for (auto iter : std::get<core::json::array_t>(value)) {
        ++currencies;
        json::Currency currency(iter);
        switch (currencies) {
          case 1:
            EXPECT_DOUBLE_EQ(currency.withdrawal_fee, 0.0004);
            EXPECT_DOUBLE_EQ(currency.min_withdrawal_fee, 0.0001);
            EXPECT_EQ(currency.min_confirmations, uint32_t{4});
            EXPECT_EQ(currency.fee_precision, uint32_t{4});
            EXPECT_EQ(currency.currency_long, "Ethereum");
            EXPECT_EQ(currency.currency, "ETH");
            EXPECT_EQ(currency.coin_type, "ETHER");
            break;
          case 2:
            EXPECT_DOUBLE_EQ(currency.withdrawal_fee, 0.0001);
            EXPECT_DOUBLE_EQ(currency.min_withdrawal_fee, 0.0001);
            EXPECT_EQ(currency.min_confirmations, uint32_t{1});
            EXPECT_EQ(currency.fee_precision, uint32_t{4});
            EXPECT_EQ(currency.currency_long, "Bitcoin");
            EXPECT_EQ(currency.currency, "BTC");
            EXPECT_EQ(currency.coin_type, "BITCOIN");
            break;
        }
      }
    }
  }
  EXPECT_EQ(results, 1);
  EXPECT_EQ(currencies, 2);
}
