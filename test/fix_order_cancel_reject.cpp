/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/fix/reader.hpp"

#include "roq/deribit/fix/order_cancel_reject.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

using OrderCancelReject = deribit::fix::OrderCancelReject;

TEST_CASE("fix_order_cancel_reject_parse_message", "[fix_order_cancel_reject]") {
  auto const message =
      "8=FIX.4.4\0019=99\00135=9\00149=DERIBITSERVER\00156=ROQ_TRADIN"
      "G\00134=3\00152=20190908-17:39:23.573\00141=123\00111=345\0013"
      "9=8\00158=not_found\00110=000\001"sv;
  int results = 0;
  auto parser = [&](auto &message_2) {
    ++results;
    CHECK(message_2.header.msg_type == roq::fix::MsgType::ORDER_CANCEL_REJECT);
    auto result = OrderCancelReject::create(message_2);
    CHECK(result.orig_cl_ord_id == "123"sv);
    CHECK(result.cl_ord_id == "345"sv);
    CHECK(result.ord_status == roq::fix::OrdStatus::REJECTED);
    CHECK(result.text == "not_found"sv);
  };
  auto bytes = roq::fix::Reader<roq::fix::Version::FIX_44>::dispatch(message, parser);
  CHECK(bytes == std::size(message));
  CHECK(results == 1);
}

TEST_CASE("fix_order_cancel_reject_already_cancelled", "[fix_order_cancel_reject]") {
  auto const message =
      "8=FIX.4.4\0019=146\00135=9\00149=DERIBITSERVER\00156=ROQ_TRADI"
      "NG\00134=58\00152=20210828-03:55:00.570\00141=5wAC6QMAAwAACDaI"
      "JMsS\00111=6446518867\00139=4\00158=already_cancelled\001151=1"
      "\0016=0.000\00110=180\001"sv;
  int results = 0;
  auto parser = [&](auto &message_2) {
    ++results;
    CHECK(message_2.header.msg_type == roq::fix::MsgType::ORDER_CANCEL_REJECT);
    auto result = OrderCancelReject::create(message_2);
    CHECK(result.orig_cl_ord_id == "5wAC6QMAAwAACDaIJMsS"sv);
    CHECK(result.cl_ord_id == "6446518867"sv);
    CHECK(result.ord_status == roq::fix::OrdStatus::CANCELED);
    CHECK(result.text == "already_cancelled"sv);
  };
  auto bytes = roq::fix::Reader<roq::fix::Version::FIX_44>::dispatch(message, parser);
  CHECK(bytes == std::size(message));
  CHECK(results == 1);
}
