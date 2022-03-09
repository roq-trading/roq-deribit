/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <catch2/catch.hpp>

#include "roq/core/fix/reader.hpp"

#include "roq/deribit/fix/position_report.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

TEST_CASE("fix_position_report_parse_message", "[fix_position_report]") {
  const auto message =
      "8=FIX.4.4\0019=245\00135=AP\00149=DERIBITSERVER\00156=ROQ_TRAD"
      "ING\00134=5\00152=20190920-17:10:28.595\001721=3221109\001710="
      "roq-pos-003\001724=0\001728=0\001702=1\001703=TQ\001704=0\0017"
      "05=0\00155=BTC-27SEP19\001854=1\001231=10.0000\001883=10184.50"
      "00\001730=0.0000\00195=11\00196=0.0;0.0;0.0\001100088=0.0000\001"
      "100089=0.00000000\00110=026\001"sv;
  core::Buffer buffer(1024 * 1024);
  core::fix::Buffer decode_buffer(buffer);
  int results = 0;
  auto bytes = core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
      [&](const core::fix::message_t &message) {
        ++results;
        CHECK(message.header.msg_type == core::fix::MsgType::POSITION_REPORT);
        auto position_report = fix::PositionReport::create(message, decode_buffer);
        CHECK(position_report.pos_maint_rpt_id == "3221109"sv);
        CHECK(position_report.pos_req_id == "roq-pos-003"sv);
        CHECK(position_report.pos_req_type == core::fix::PosReqType::POSITIONS);
        CHECK(position_report.pos_req_result == core::fix::PosReqResult::VALID);
        CHECK(std::size(position_report.no_positions) == size_t{1});
        auto &item = position_report.no_positions[0];
        CHECK(item.long_qty == 0.0_a);
        CHECK(item.short_qty == 0.0_a);
        CHECK(item.symbol == "BTC-27SEP19"sv);
        CHECK(item.qty_type == core::fix::QtyType::CONTRACTS);
        CHECK(item.contract_multiplier == 10.0_a);
        CHECK(item.underlying_end_price == 10184.50_a);
        CHECK(item.settl_price == 0.0_a);
      },
      message);
  CHECK(bytes == std::size(message));
  CHECK(results == 1);
}
