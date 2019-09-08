/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/fix/reader.h"

#include "roq/deribit/fix/position_report.h"

using namespace roq;  // NOLINT
using namespace roq::deribit;  // NOLINT

TEST(fix_position_report, parse_message) {
  const char *message =
    "8=FIX.4.4\0019=230\00135=AP\00149=DERIBITSERVER\00156=ROQ_TRAD"
    "ING\00134=4\00152=20190908-15:21:54.384\001721=2957706\001710="
    "123\001724=0\001728=0\001702=1\001704=0\001705=0\00155=BTC-27S"
    "EP19\001854=1\001231=10.0000\001883=10510.3400\001730=0.0000\001"
    "95=11\00196=0.0;0.0;0.0\001100088=0.0000\001100089=0.00000000\001"
    "10=169\001";
  std::vector<std::byte> buffer(1024 * 1024);
  int results = 0;
  auto bytes = core::fix::Reader::dispatch(
      [&](const core::fix::message_t& message) {
        ++results;
        EXPECT_EQ(message.header.msg_type, core::fix::MsgType::POSITION_REPORT);
        auto position_report = fix::PositionReport::parse(message, buffer);
        EXPECT_EQ(position_report.pos_maint_rpt_id, "2957706");
        EXPECT_EQ(position_report.pos_req_id, "123");
        EXPECT_EQ(position_report.pos_req_type, core::fix::PosReqType::POSITIONS);
        EXPECT_EQ(position_report.pos_req_result, core::fix::PosReqResult::VALID);
        EXPECT_EQ(position_report.positions.length, size_t{1});
        auto& item = position_report.positions.items[0];
        EXPECT_DOUBLE_EQ(item.long_qty, 0.0);
        EXPECT_DOUBLE_EQ(item.short_qty, 0.0);
        EXPECT_EQ(item.symbol, "BTC-27SEP19");
        EXPECT_EQ(item.qty_type, core::fix::QtyType::CONTRACTS);
        EXPECT_DOUBLE_EQ(item.contract_multiplier, 10.0);
        EXPECT_DOUBLE_EQ(item.underlying_price, 10510.34);
        EXPECT_DOUBLE_EQ(item.settl_price, 0.0);
      },
      message,
      std::strlen(message));
  EXPECT_EQ(bytes, std::strlen(message));
  EXPECT_EQ(results, 1);
}
