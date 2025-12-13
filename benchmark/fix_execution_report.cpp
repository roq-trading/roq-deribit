/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/fix/reader.hpp"

#include "roq/deribit/fix/execution_report.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using ExecutionReport = deribit::fix::ExecutionReport;

namespace {
auto const MESSAGE =
    "8=FIX.4.4\0019=275\00135=8\00149=DERIBITSERVER\00156=ROQ_TRADI"
    "NG\00134=2\00152=20190908-17:18:38.983\00137=2831903667\00111="
    "2831903667\00141=123\001150=I\00139=4\00154=1\00160=20190908-1"
    "7:18:38.983\001151=1\00114=0\00138=1\00140=2\00144=0.5000\0011"
    "03=0\00158=success\001207=DERIBITSERVER\00155=BTC-27SEP19\0018"
    "54=1\001231=10.0000\0016=0.000\001210=1\001100010=roq;123;345\001"
    "10=195\001"sv;
}  // namespace

void BM_fix_execution_report_parse_message(benchmark::State &state) {
  std::vector<std::byte> buffer(8192);
  uint64_t processed = 0;
  auto parser = [&](auto &message_2) {
    auto result = ExecutionReport::create(message_2, buffer);
    if (!std::empty(result.order_id)) {
      ++processed;
    }
  };
  for (auto _ : state) {
    roq::fix::Reader<roq::fix::Version::FIX_44>::dispatch(MESSAGE, parser);
  }
}

BENCHMARK(BM_fix_execution_report_parse_message);
