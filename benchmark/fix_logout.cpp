/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/fix/reader.hpp"

#include "roq/deribit/fix/logout.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using Logout = deribit::fix::Logout;

namespace {
auto const MESSAGE =
    "8=FIX.4.4\0019=90\00135=5\00149=DERIBITSERVER\00156=ROQ_TRADIN"
    "G\00134=1\00152=20190907-16:56:43.398\00158=invalid_credential"
    "s\00110=166\001"sv;
}  // namespace

void BM_fix_logout_parse_message(benchmark::State &state) {
  uint64_t processed = 0;
  auto parser = [&](auto &message_2) {
    auto logout = Logout::create(message_2);
    if (!std::empty(logout.text)) {
      ++processed;
    }
  };
  for (auto _ : state) {
    roq::fix::Reader<roq::fix::Version::FIX_44>::dispatch(MESSAGE, parser);
  }
}

BENCHMARK(BM_fix_logout_parse_message);
