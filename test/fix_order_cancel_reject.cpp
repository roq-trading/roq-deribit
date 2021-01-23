/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/fix/reader.h"

#include "roq/deribit/fix/order_cancel_reject.h"

using namespace roq;           // NOLINT
using namespace roq::deribit;  // NOLINT

TEST(fix_order_cancel_reject, parse_message) {
  const char *message =
      "8=FIX.4.4\0019=99\00135=9\00149=DERIBITSERVER\00156=ROQ_TRADIN"
      "G\00134=3\00152=20190908-17:39:23.573\00141=123\00111=345\0013"
      "9=8\00158=not_found\00110=000\001";
  int results = 0;
  auto bytes = core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
      [&](const core::fix::message_t &message) {
        ++results;
        EXPECT_EQ(
            message.header.msg_type, core::fix::MsgType::ORDER_CANCEL_REJECT);
        auto result = fix::OrderCancelReject::create(message);
        EXPECT_EQ(result.orig_cl_ord_id, "123");
        EXPECT_EQ(result.cl_ord_id, "345");
        EXPECT_EQ(result.ord_status, core::fix::OrdStatus::REJECTED);
        EXPECT_EQ(result.text, "not_found");
      },
      message,
      std::strlen(message));
  EXPECT_EQ(bytes, std::strlen(message));
  EXPECT_EQ(results, 1);
}
