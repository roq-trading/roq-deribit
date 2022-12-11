/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/deribit/fix/heartbeat.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

namespace {
auto const MESSAGE =
    "8=FIX.4.4\0019=89\00135=0\00149=DERIBITSERVER\00156=ROQ_TRADIN"
    "G\00134=2\00152=20190908-08:47:31.503\001112=anybody in there?"
    "\00110=084\001"sv;
}  // namespace

// cppcheck-suppress constParameterCallback
void BM_fix_heartbeat_parse_message(benchmark::State &state) {
  uint64_t processed = 0;
  for (auto _ : state) {
    core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
        [&](core::fix::Message const &message) {
          auto heartbeat = fix::Heartbeat::create(message);
          if (std::empty(heartbeat.test_req_id))
            ++processed;
        },
        MESSAGE);
  }
}

BENCHMARK(BM_fix_heartbeat_parse_message);
