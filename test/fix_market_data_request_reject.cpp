/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/fix/reader.hpp"

#include "roq/deribit/fix/market_data_request_reject.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

using MarketDataRequestReject = deribit::fix::MarketDataRequestReject;

TEST_CASE("fix_market_data_request_reject_parse_message", "[fix_market_data_request_reject]") {
  auto const message =
      "8=FIX.4.4\0019=102\00135=Y\00149=DERIBITSERVER\00156=ROQ_TRADI"
      "NG\00134=4\00152=20190908-10:54:45.738\001262=123\00158=unknow"
      "n Symbol: BTC-XXX\00110=152\001"sv;
  int results = 0;
  auto parser = [&](auto &message_2) {
    ++results;
    CHECK(message_2.header.msg_type == roq::fix::MsgType::MARKET_DATA_REQUEST_REJECT);
    auto reject = MarketDataRequestReject::create(message_2);
    CHECK(reject.md_req_id == "123"sv);
    CHECK(reject.md_req_rej_reason == roq::fix::MDReqRejReason::UNDEFINED);
    CHECK(reject.text == "unknown Symbol: BTC-XXX"sv);
  };
  auto bytes = roq::fix::Reader<roq::fix::Version::FIX_44>::dispatch(message, parser);
  CHECK(bytes == std::size(message));
  CHECK(results == 1);
}
