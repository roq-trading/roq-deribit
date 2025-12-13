/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/fix/reader.hpp"

#include "roq/deribit/fix/position_report.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

using PositionReport = deribit::fix::PositionReport;

TEST_CASE("fix_position_report_parse_message", "[fix_position_report]") {
  auto const message =
      "8=FIX.4.4\0019=245\00135=AP\00149=DERIBITSERVER\00156=ROQ_TRAD"
      "ING\00134=5\00152=20190920-17:10:28.595\001721=3221109\001710="
      "roq-pos-003\001724=0\001728=0\001702=1\001703=TQ\001704=0\0017"
      "05=0\00155=BTC-27SEP19\001854=1\001231=10.0000\001883=10184.50"
      "00\001730=0.0000\00195=11\00196=0.0;0.0;0.0\001100088=0.0000\001"
      "100089=0.00000000\00110=026\001"sv;
  std::vector<std::byte> buffer(1'048'576);
  int results = 0;
  auto parser = [&](auto &message_2) {
    ++results;
    CHECK(message_2.header.msg_type == roq::fix::MsgType::POSITION_REPORT);
    auto position_report = PositionReport::create(message_2, buffer);
    CHECK(position_report.pos_maint_rpt_id == "3221109"sv);
    CHECK(position_report.pos_req_id == "roq-pos-003"sv);
    CHECK(position_report.pos_req_type == roq::fix::PosReqType::POSITIONS);
    CHECK(position_report.pos_req_result == roq::fix::PosReqResult::VALID);
    CHECK(std::size(position_report.no_positions) == size_t{1});
    auto &item = position_report.no_positions[0];
    CHECK(item.long_qty == 0.0_a);
    CHECK(item.short_qty == 0.0_a);
    CHECK(item.symbol == "BTC-27SEP19"sv);
    CHECK(item.qty_type == roq::fix::QtyType::CONTRACTS);
    CHECK(item.contract_multiplier == 10.0_a);
    CHECK(item.underlying_end_price == 10184.50_a);
    CHECK(item.settl_price == 0.0_a);
  };
  auto bytes = roq::fix::Reader<roq::fix::Version::FIX_44>::dispatch(message, parser);
  CHECK(bytes == std::size(message));
  CHECK(results == 1);
}
