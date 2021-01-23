/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/fix/reader.h"

#include "roq/deribit/fix/heartbeat.h"

using namespace roq;           // NOLINT
using namespace roq::deribit;  // NOLINT

TEST(fix_heartbeat, parse_message) {
  const char *message =
      "8=FIX.4.4\0019=89\00135=0\00149=DERIBITSERVER\00156=ROQ_TRADIN"
      "G\00134=2\00152=20190908-08:47:31.503\001112=anybody in there?"
      "\00110=084\001";
  int results = 0;
  auto bytes = core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
      [&](const core::fix::message_t &message) {
        ++results;
        EXPECT_EQ(message.header.msg_type, core::fix::MsgType::HEARTBEAT);
        auto heartbeat = fix::Heartbeat::create(message);
        EXPECT_EQ(heartbeat.test_req_id, "anybody in there?");
      },
      message,
      std::strlen(message));
  EXPECT_EQ(bytes, std::strlen(message));
  EXPECT_EQ(results, 1);
}
