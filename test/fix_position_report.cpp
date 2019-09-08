/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/fix/reader.h"

#include "roq/deribit/fix/position_report.h"

using namespace roq;  // NOLINT
using namespace roq::deribit;  // NOLINT

TEST(fix_position_report, parse_message) {
  const char *message =
    "8=FIX.4.4\0019=106\00135=AP\00149=DERIBITSERVER\00156=ROQ_TRAD"
    "ING\00134=4\00152=20190908-08:47:31.543\001721=2950652\001710="
    "123\001724=0\001728=0\001702=0\00110=125\001";
  std::vector<std::byte> buffer(1024 * 1024);
  int results = 0;
  auto bytes = core::fix::Reader::dispatch(
      [&](const core::fix::message_t& message) {
        ++results;
        EXPECT_EQ(message.header.msg_type, core::fix::MsgType::POSITION_REPORT);
        auto position_report = fix::PositionReport::parse(message, buffer);
        EXPECT_EQ(position_report.pos_maint_rpt_id, "2950652");
        EXPECT_EQ(position_report.pos_req_id, "123");
        EXPECT_EQ(position_report.pos_req_type, core::fix::PosReqType::POSITIONS);
        EXPECT_EQ(position_report.pos_req_result, core::fix::PosReqResult::VALID);
        EXPECT_EQ(position_report.positions.length, size_t{0});
      },
      message,
      std::strlen(message));
  EXPECT_EQ(bytes, std::strlen(message));
  EXPECT_EQ(results, 1);
}
