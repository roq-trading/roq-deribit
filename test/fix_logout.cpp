/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/fix/reader.h"

#include "roq/deribit/fix/logout.h"

using namespace roq;  // NOLINT
using namespace roq::deribit;  // NOLINT

TEST(fix_logout, parse_message) {
  const char *message =
    "8=FIX.4.4\0019=90\00135=5\00149=DERIBITSERVER\00156=ROQ_TRADIN"
    "G\00134=1\00152=20190907-16:56:43.398\00158=invalid_credential"
    "s\00110=166\001";
  int results = 0;
  auto bytes = core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
      [&](const core::fix::message_t& message) {
        ++results;
        EXPECT_EQ(message.header.msg_type, core::fix::MsgType::LOGOUT);
        auto logout = fix::Logout::create(message);
        EXPECT_EQ(logout.text, "invalid_credentials");
      },
      message,
      std::strlen(message));
  EXPECT_EQ(bytes, std::strlen(message));
  EXPECT_EQ(results, 1);
}
