/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/deribit/fix/heartbeat.h"

using namespace roq;  // NOLINT
using namespace roq::deribit;  // NOLINT

namespace {
static const char *MESSAGE =
  "8=FIX.4.4\0019=89\00135=0\00149=DERIBITSERVER\00156=ROQ_TRADIN"
  "G\00134=2\00152=20190908-08:47:31.503\001112=anybody in there?"
  "\00110=084\001";
}  // namespace

void BM_fix_heartbeat_parse_message(benchmark::State& state) {
  uint64_t processed = 0;
  for (auto _ : state) {
    core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
        [&](const core::fix::message_t& message) {
          auto heartbeat = fix::Heartbeat::create(message);
          if (heartbeat.test_req_id.empty())
            ++processed;
        },
        MESSAGE,
        std::strlen(MESSAGE));
  }
}

BENCHMARK(BM_fix_heartbeat_parse_message);
