/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/fix/reader.h"

#include "roq/deribit/fix/order_cancel_reject.h"

using namespace roq;
using namespace roq::deribit;

TEST(fix_order_cancel_reject, parse_message) {
  const auto message =
      "8=FIX.4.4\0019=99\00135=9\00149=DERIBITSERVER\00156=ROQ_TRADIN"
      "G\00134=3\00152=20190908-17:39:23.573\00141=123\00111=345\0013"
      "9=8\00158=not_found\00110=000\001"_sv;
  int results = 0;
  auto bytes = core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
      [&](const core::fix::message_t &message) {
        ++results;
        EXPECT_EQ(message.header.msg_type, core::fix::MsgType::ORDER_CANCEL_REJECT);
        auto result = fix::OrderCancelReject::create(message);
        EXPECT_EQ(result.orig_cl_ord_id, "123"_sv);
        EXPECT_EQ(result.cl_ord_id, "345"_sv);
        EXPECT_EQ(result.ord_status, core::fix::OrdStatus::REJECTED);
        EXPECT_EQ(result.text, "not_found"_sv);
      },
      message);
  EXPECT_EQ(bytes, std::size(message));
  EXPECT_EQ(results, 1);
}

TEST(fix_order_cancel_reject, already_cancelled) {
  const auto message =
      "8=FIX.4.4\0019=146\00135=9\00149=DERIBITSERVER\00156=ROQ_TRADI"
      "NG\00134=58\00152=20210828-03:55:00.570\00141=5wAC6QMAAwAACDaI"
      "JMsS\00111=6446518867\00139=4\00158=already_cancelled\001151=1"
      "\0016=0.000\00110=180\001"_sv;
  int results = 0;
  auto bytes = core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
      [&](const core::fix::message_t &message) {
        ++results;
        EXPECT_EQ(message.header.msg_type, core::fix::MsgType::ORDER_CANCEL_REJECT);
        auto result = fix::OrderCancelReject::create(message);
        EXPECT_EQ(result.orig_cl_ord_id, "5wAC6QMAAwAACDaIJMsS"_sv);
        EXPECT_EQ(result.cl_ord_id, "6446518867"_sv);
        EXPECT_EQ(result.ord_status, core::fix::OrdStatus::CANCELED);
        EXPECT_EQ(result.text, "already_cancelled"_sv);
      },
      message);
  EXPECT_EQ(bytes, std::size(message));
  EXPECT_EQ(results, 1);
}
