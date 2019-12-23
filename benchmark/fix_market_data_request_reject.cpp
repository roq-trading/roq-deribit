/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/patterns.h"

#include "roq/deribit/fix/parser.h"

using namespace roq;  // NOLINT
using namespace roq::deribit;  // NOLINT

namespace {
static const char *MESSAGE =
  "8=FIX.4.4\0019=102\00135=Y\00149=DERIBITSERVER\00156=ROQ_TRADI"
  "NG\00134=4\00152=20190908-10:54:45.738\001262=123\00158=unknow"
  "n Symbol: BTC-XXX\00110=152\001";
}  // namespace

void BM_fix_market_data_request_reject_parse_message(benchmark::State& state) {
  uint64_t processed = 0;
  for (auto _ : state) {
    core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
        [&](const core::fix::message_t& message) {
          auto market_data_request_reject = fix::MarketDataRequestReject::parse(message);
          if (!market_data_request_reject.text.empty())
            ++processed;
        },
        MESSAGE,
        std::strlen(MESSAGE));
  }
}

BENCHMARK(BM_fix_market_data_request_reject_parse_message);

void BM_fix_parser_dispatch_market_data_request_reject(benchmark::State& state) {
  std::vector<std::byte> buffer(8192);
  uint64_t processed = 0;
  for (auto _ : state) {
    core::fix::Buffer decode_buffer(buffer);
    core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
        [&](const core::fix::message_t& message) {
          fix::Parser::dispatch(
              overloaded {
                [](const fix::ExecutionReport&) {
                },
                [](const fix::Heartbeat&) {
                },
                [](const fix::Logon&) {
                },
                [](const fix::Logout&) {
                },
                [](const fix::MarketDataIncrementalRefresh&) {
                },
                [&](const fix::MarketDataRequestReject& market_data_request_reject) {
                  if (!market_data_request_reject.text.empty())
                    ++processed;
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
              decode_buffer);
        },
        MESSAGE,
        std::strlen(MESSAGE));
  }
}

BENCHMARK(BM_fix_parser_dispatch_market_data_request_reject);
