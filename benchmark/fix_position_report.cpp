/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/fix/reader.hpp"

#include "roq/deribit/fix/position_report.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using PositionReport = deribit::fix::PositionReport;

namespace {
auto const MESSAGE =
    "8=FIX.4.4\0019=245\00135=AP\00149=DERIBITSERVER\00156=ROQ_TRAD"
    "ING\00134=5\00152=20190920-17:10:28.595\001721=3221109\001710="
    "roq-pos-003\001724=0\001728=0\001702=1\001703=TQ\001704=0\0017"
    "05=0\00155=BTC-27SEP19\001854=1\001231=10.0000\001883=10184.50"
    "00\001730=0.0000\00195=11\00196=0.0;0.0;0.0\001100088=0.0000\001"
    "100089=0.00000000\00110=026\001"sv;
}  // namespace

void BM_fix_position_report_parse_message(benchmark::State &state) {
  std::vector<std::byte> buffer(8192);
  uint64_t processed = 0;
  auto parser = [&](auto &message_2) {
    auto position_report = PositionReport::create(message_2, buffer);
    if (!std::empty(position_report.pos_req_id)) {
      ++processed;
    }
  };
  for (auto _ : state) {
    roq::fix::Reader<roq::fix::Version::FIX_44>::dispatch(MESSAGE, parser);
  }
}

BENCHMARK(BM_fix_position_report_parse_message);
