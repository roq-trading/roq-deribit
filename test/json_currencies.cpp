/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <catch2/catch.hpp>

#include "roq/deribit/json/currency.h"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

TEST_CASE("json_currency_parse_message", "json_currency") {
  const auto message =
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
      R"(})"sv;

  int results = 0, currencies = 0;
  core::json::Parser parser(message);
  auto root = parser.root();
  for (auto [key, value] : std::get<core::json::object_t>(root)) {
    if (key.compare("result"sv) == 0) {
      ++results;
      for (auto iter : std::get<core::json::array_t>(value)) {
        ++currencies;
        json::Currency currency(iter);
        switch (currencies) {
          case 1:
            CHECK(currency.withdrawal_fee == 0.0004_a);
            CHECK(currency.min_withdrawal_fee == 0.0001_a);
            CHECK(currency.min_confirmations == uint32_t{4});
            CHECK(currency.fee_precision == uint32_t{4});
            CHECK(currency.currency_long == "Ethereum"sv);
            CHECK(currency.currency == "ETH"sv);
            CHECK(currency.coin_type == "ETHER"sv);
            break;
          case 2:
            CHECK(currency.withdrawal_fee == 0.0001_a);
            CHECK(currency.min_withdrawal_fee == 0.0001_a);
            CHECK(currency.min_confirmations == uint32_t{1});
            CHECK(currency.fee_precision == uint32_t{4});
            CHECK(currency.currency_long == "Bitcoin"sv);
            CHECK(currency.currency == "BTC"sv);
            CHECK(currency.coin_type == "BITCOIN"sv);
            break;
        }
      }
    }
  }
  CHECK(results == 1);
  CHECK(currencies == 2);
}
