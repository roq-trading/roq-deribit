/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/fix/reader.h"

#include "roq/deribit/fix/reject.h"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

TEST(fix_reject, parse_message) {
  const auto message =
      "8=FIX.4.4\0019=98\00135=3\00149=DERIBITSERVER\00156=ROQ_TRADIN"
      "G\00134=5\00152=20190908-08:47:31.543\00145=5\001372=AN\00158="
      "not_implemented\00110=092\001"sv;
  int results = 0;
  auto bytes = core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
      [&](const core::fix::message_t &message) {
        ++results;
        EXPECT_EQ(message.header.msg_type, core::fix::MsgType::REJECT);
        auto reject = fix::Reject::create(message);
        EXPECT_EQ(reject.ref_seq_num, uint64_t{5});
        EXPECT_EQ(reject.ref_msg_type, core::fix::MsgType::REQUEST_FOR_POSITIONS);
        EXPECT_EQ(reject.text, "not_implemented"sv);
      },
      message);
  EXPECT_EQ(bytes, std::size(message));
  EXPECT_EQ(results, 1);
}
