/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/fix/reader.h"

#include "roq/deribit/fix/position_report.h"

using namespace roq;  // NOLINT
using namespace roq::deribit;  // NOLINT

TEST(fix_position_report, parse_message) {
  const char *message =
    "8=FIX.4.4\0019=245\00135=AP\00149=DERIBITSERVER\00156=ROQ_TRAD"
    "ING\00134=5\00152=20190920-17:10:28.595\001721=3221109\001710="
    "roq-pos-003\001724=0\001728=0\001702=1\001703=TQ\001704=0\0017"
    "05=0\00155=BTC-27SEP19\001854=1\001231=10.0000\001883=10184.50"
    "00\001730=0.0000\00195=11\00196=0.0;0.0;0.0\001100088=0.0000\001"
    "100089=0.00000000\00110=026\001";
  std::vector<std::byte> buffer(1024 * 1024);
  core::fix::Buffer decode_buffer(buffer);
  int results = 0;
  auto bytes = core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
      [&](const core::fix::message_t& message) {
        ++results;
        EXPECT_EQ(message.header.msg_type, core::fix::MsgType::POSITION_REPORT);
        auto position_report = fix::PositionReport::parse(message, decode_buffer);
        EXPECT_EQ(position_report.pos_maint_rpt_id, "3221109");
        EXPECT_EQ(position_report.pos_req_id, "roq-pos-003");
        EXPECT_EQ(position_report.pos_req_type, core::fix::PosReqType::POSITIONS);
        EXPECT_EQ(position_report.pos_req_result, core::fix::PosReqResult::VALID);
        EXPECT_EQ(position_report.positions.length, size_t{1});
        auto& item = position_report.positions.items[0];
        EXPECT_DOUBLE_EQ(item.long_qty, 0.0);
        EXPECT_DOUBLE_EQ(item.short_qty, 0.0);
        EXPECT_EQ(item.symbol, "BTC-27SEP19");
        EXPECT_EQ(item.qty_type, core::fix::QtyType::CONTRACTS);
        EXPECT_DOUBLE_EQ(item.contract_multiplier, 10.0);
        EXPECT_DOUBLE_EQ(item.underlying_price, 10184.50);
        EXPECT_DOUBLE_EQ(item.settl_price, 0.0);
      },
      message,
      std::strlen(message));
  EXPECT_EQ(bytes, std::strlen(message));
  EXPECT_EQ(results, 1);
}
