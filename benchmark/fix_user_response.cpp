/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/fix/reader.hpp"

#include "roq/deribit/fix/user_response.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using UserResponse = deribit::fix::UserResponse;

namespace {
auto const MESSAGE =
    "8=FIX.4.4\0019=199\00135=BF\00149=DERIBITSERVER\00156=ROQ_TRAD"
    "ING\00134=3\00152=20190908-08:47:31.511\001923=123\001553=5MP4"
    "0u9h\001926=1\00115=BTC\001100001=10.0\001100002=10.0\00110000"
    "3=0.0000\001100004=0.0000\001100005=0.0\001100006=0.0\00110001"
    "1=0.0\001100013=10.0\00110=004\001"sv;
}  // namespace

void BM_fix_user_response_parse_message(benchmark::State &state) {
  uint64_t processed = 0;
  auto parser = [&](auto &message_2) {
    auto user_response = UserResponse::create(message_2);
    if (!std::empty(user_response.user_request_id)) {
      ++processed;
    }
  };
  for (auto _ : state) {
    roq::fix::Reader<roq::fix::Version::FIX_44>::dispatch(MESSAGE, parser);
  }
}

BENCHMARK(BM_fix_user_response_parse_message);
