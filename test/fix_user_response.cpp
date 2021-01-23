/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/fix/reader.h"

#include "roq/deribit/fix/user_response.h"

using namespace roq;           // NOLINT
using namespace roq::deribit;  // NOLINT

TEST(fix_user_response, parse_message) {
  const char *message =
      "8=FIX.4.4\0019=199\00135=BF\00149=DERIBITSERVER\00156=ROQ_TRAD"
      "ING\00134=3\00152=20190908-08:47:31.511\001923=123\001553=5MP4"
      "0u9h\001926=1\00115=BTC\001100001=10.0\001100002=10.0\00110000"
      "3=0.0000\001100004=0.0000\001100005=0.0\001100006=0.0\00110001"
      "1=0.0\001100013=10.0\00110=004\001";
  int results = 0;
  auto bytes = core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
      [&](const core::fix::message_t &message) {
        ++results;
        EXPECT_EQ(message.header.msg_type, core::fix::MsgType::USER_RESPONSE);
        auto user_response = fix::UserResponse::create(message);
        EXPECT_EQ(user_response.user_request_id, "123");
        EXPECT_EQ(user_response.username, "5MP40u9h");
        EXPECT_EQ(user_response.user_status, core::fix::UserStatus::LOGGED_IN);
        EXPECT_EQ(user_response.currency, "BTC");
        EXPECT_DOUBLE_EQ(user_response.deribit_user_equity, 10.0);
        EXPECT_DOUBLE_EQ(user_response.deribit_user_balance, 10.0);
        EXPECT_DOUBLE_EQ(user_response.deribit_user_initial_margin, 0.0);
        EXPECT_DOUBLE_EQ(user_response.deribit_user_maintenance_margin, 0.0);
        EXPECT_DOUBLE_EQ(user_response.deribit_user_unrealized_pl, 0.0);
        EXPECT_DOUBLE_EQ(user_response.deribit_user_realized_pl, 0.0);
        EXPECT_DOUBLE_EQ(user_response.deribit_user_total_pl, 0.0);
        EXPECT_DOUBLE_EQ(user_response.deribit_user_margin_balance, 10.0);
      },
      message,
      std::strlen(message));
  EXPECT_EQ(bytes, std::strlen(message));
  EXPECT_EQ(results, 1);
}
