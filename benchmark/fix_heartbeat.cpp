/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/fix/reader.hpp"

#include "roq/deribit/fix/heartbeat.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using Heartbeat = deribit::fix::Heartbeat;

namespace {
auto const MESSAGE =
    "8=FIX.4.4\0019=89\00135=0\00149=DERIBITSERVER\00156=ROQ_TRADIN"
    "G\00134=2\00152=20190908-08:47:31.503\001112=anybody in there?"
    "\00110=084\001"sv;
}  // namespace

void BM_fix_heartbeat_parse_message(benchmark::State &state) {
  uint64_t processed = 0;
  auto parser = [&](auto &message_2) {
    auto heartbeat = Heartbeat::create(message_2);
    if (std::empty(heartbeat.test_req_id)) {
      ++processed;
    }
  };
  for (auto _ : state) {
    roq::fix::Reader<roq::fix::Version::FIX_44>::dispatch(MESSAGE, parser);
  }
}

BENCHMARK(BM_fix_heartbeat_parse_message);
