/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/fix/reader.h"

#include "roq/deribit/fix/logon.h"

using namespace roq;  // NOLINT
using namespace roq::deribit;  // NOLINT

TEST(fix_logon, parse_message) {
  const char *message =
    "8=FIX.4.4\0019=211\00135=A\00149=DERIBITSERVER\00156=ROQ_TRADI"
    "NG\00134=1\00152=20190907-16:45:58.192\001108=10\00195=58\0019"
    "6=1567874758168.y4/hA3i6qxm4yVL+3N7IrGcINVAFMLFhy4l7ATSehxc=\001"
    "553=5MP40u9h\001554=j/tVe9IsQuc+RjegscnHcJ6czMVNM1+ib7vjbY3UV0"
    "M=\0019001=Y\00110=115\001";
  int results = 0;
  auto bytes = core::fix::Reader::dispatch(
      [&](const core::fix::message_t& message) {
        ++results;
        EXPECT_EQ(message.header.msg_type, core::fix::MsgType::LOGON);
        auto result = fix::Logon::parse(message);
        EXPECT_EQ(result.heart_bt_int, uint32_t{10});
        EXPECT_EQ(result.raw_data, "1567874758168.y4/hA3i6qxm4yVL+3N7IrGcINVAFMLFhy4l7ATSehxc=");
        EXPECT_EQ(result.username, "5MP40u9h");
        EXPECT_EQ(result.password, "j/tVe9IsQuc+RjegscnHcJ6czMVNM1+ib7vjbY3UV0M=");
        EXPECT_EQ(result.deribit_cancel_on_disconnect, true);
        EXPECT_EQ(result.deribit_use_wordsafe_tags, false);
      },
      message,
      std::strlen(message));
  EXPECT_EQ(bytes, std::strlen(message));
  EXPECT_EQ(results, 1);
}
