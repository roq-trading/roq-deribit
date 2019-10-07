/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/patterns.h"

#include "roq/deribit/fix/parser.h"

using namespace roq;  // NOLINT
using namespace roq::deribit;  // NOLINT

namespace {
static const char *MESSAGE =
  "8=FIX.4.4\0019=275\00135=8\00149=DERIBITSERVER\00156=ROQ_TRADI"
  "NG\00134=2\00152=20190908-17:18:38.983\00137=2831903667\00111="
  "2831903667\00141=123\001150=I\00139=4\00154=1\00160=20190908-1"
  "7:18:38.983\001151=1\00114=0\00138=1\00140=2\00144=0.5000\0011"
  "03=0\00158=success\001207=DERIBITSERVER\00155=BTC-27SEP19\0018"
  "54=1\001231=10.0000\0016=0.000\001210=1\001100010=roq;123;345\001"
  "10=195\001";
}  // namespace

void BM_fix_execution_report_parse_message(benchmark::State& state) {
  uint64_t processed = 0;
  for (auto _ : state) {
    core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
        [&](const core::fix::message_t& message) {
          auto result = fix::ExecutionReport::parse(message);
          if (!result.order_id.empty())
            ++processed;
        },
        MESSAGE,
        std::strlen(MESSAGE));
  }
}

BENCHMARK(BM_fix_execution_report_parse_message);

void BM_fix_parser_dispatch_execution_report(benchmark::State& state) {
  std::vector<std::byte> buffer(8192);
  uint64_t processed = 0;
  for (auto _ : state) {
    core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
        [&](const core::fix::message_t& message) {
          fix::Parser::dispatch(
              overloaded {
                [&](const fix::ExecutionReport& execution_report) {
                  if (!execution_report.order_id.empty())
                    ++processed;
                },
                [](const fix::Heartbeat&) {
                },
                [](const fix::Logon&) {
                },
                [](const fix::Logout&) {
                },
                [](const fix::MarketDataIncrementalRefresh&) {
                },
                [](const fix::MarketDataRequestReject&) {
                },
                [](const fix::MarketDataSnapshotFullRefresh&) {
                },
                [](const fix::OrderCancelReject&) {
                },
                [](const fix::PositionReport&) {
                },
                [](const fix::Reject&) {
                },
                [](const fix::ResendRequest&) {
                },
                [](const fix::SecurityList&) {
                },
                [](const fix::TestRequest&) {
                },
                [](const fix::UserResponse&) {
                },
              },
              message,
              buffer);
        },
        MESSAGE,
        std::strlen(MESSAGE));
  }
}

BENCHMARK(BM_fix_parser_dispatch_execution_report);
