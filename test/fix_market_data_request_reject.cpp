/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <catch2/catch.hpp>

#include "roq/core/fix/reader.h"

#include "roq/deribit/fix/market_data_request_reject.h"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

TEST_CASE("fix_market_data_request_reject_parse_message", "[fix_market_data_request_reject]") {
  const auto message =
      "8=FIX.4.4\0019=102\00135=Y\00149=DERIBITSERVER\00156=ROQ_TRADI"
      "NG\00134=4\00152=20190908-10:54:45.738\001262=123\00158=unknow"
      "n Symbol: BTC-XXX\00110=152\001"sv;
  int results = 0;
  auto bytes = core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
      [&](const core::fix::message_t &message) {
        ++results;
        CHECK(message.header.msg_type == core::fix::MsgType::MARKET_DATA_REQUEST_REJECT);
        auto reject = fix::MarketDataRequestReject::create(message);
        CHECK(reject.md_req_id == "123"sv);
        CHECK(reject.md_req_rej_reason == core::fix::MDReqRejReason::UNKNOWN);
        CHECK(reject.text == "unknown Symbol: BTC-XXX"sv);
      },
      message);
  CHECK(bytes == std::size(message));
  CHECK(results == 1);
}
