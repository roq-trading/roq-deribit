/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/deribit/fix/position_report.h"

using namespace roq;  // NOLINT
using namespace roq::deribit;  // NOLINT

namespace {
static const char *MESSAGE =
  "8=FIX.4.4\0019=245\00135=AP\00149=DERIBITSERVER\00156=ROQ_TRAD"
  "ING\00134=5\00152=20190920-17:10:28.595\001721=3221109\001710="
  "roq-pos-003\001724=0\001728=0\001702=1\001703=TQ\001704=0\0017"
  "05=0\00155=BTC-27SEP19\001854=1\001231=10.0000\001883=10184.50"
  "00\001730=0.0000\00195=11\00196=0.0;0.0;0.0\001100088=0.0000\001"
  "100089=0.00000000\00110=026\001";
}  // namespace

void BM_fix_position_report_parse_message(benchmark::State& state) {
  core::utils::Buffer buffer(8192);
  uint64_t processed = 0;
  for (auto _ : state) {
    core::fix::Buffer decode_buffer(buffer);
    core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
        [&](const core::fix::message_t& message) {
          auto position_report = fix::PositionReport::create(message, decode_buffer);
          if (!position_report.pos_req_id.empty())
            ++processed;
        },
        MESSAGE,
        std::strlen(MESSAGE));
  }
}

BENCHMARK(BM_fix_position_report_parse_message);
