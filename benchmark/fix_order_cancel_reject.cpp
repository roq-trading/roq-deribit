/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/patterns.h"

#include "roq/deribit/fix/parser.h"

using namespace roq;  // NOLINT
using namespace roq::deribit;  // NOLINT

namespace {
static const char *MESSAGE =
  "8=FIX.4.4\0019=99\00135=9\00149=DERIBITSERVER\00156=ROQ_TRADIN"
  "G\00134=3\00152=20190908-17:39:23.573\00141=123\00111=345\0013"
  "9=8\00158=not_found\00110=000\001";
}  // namespace

void BM_fix_order_cancel_reject_parse_message(benchmark::State& state) {
  uint64_t processed = 0;
  for (auto _ : state) {
    auto bytes = core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
        [&](const core::fix::message_t& message) {
          auto result = fix::OrderCancelReject::parse(message);
          if (!result.text.empty())
            ++processed;
        },
        MESSAGE,
        std::strlen(MESSAGE));
  }
}

BENCHMARK(BM_fix_order_cancel_reject_parse_message);

void BM_fix_parser_dispatch_order_cancel_reject(benchmark::State& state) {
  std::vector<std::byte> buffer(8192);
  uint64_t processed = 0;
  for (auto _ : state) {
    auto bytes = core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
        [&](const core::fix::message_t& message) {
          fix::Parser::dispatch(
              overloaded {
                [](const fix::ExecutionReport& execution_report) {
                },
                [](const fix::Heartbeat& heartbeat) {
                },
                [](const fix::Logon& logon) {
                },
                [](const fix::Logout& logout) {
                },
                [](const fix::MarketDataIncrementalRefresh& market_data_incremental_refresh) {
                },
                [](const fix::MarketDataRequestReject& market_data_request_reject) {
                },
                [](const fix::MarketDataSnapshotFullRefresh& market_data_snapshot_full_refresh) {
                },
                [&](const fix::OrderCancelReject& order_cancel_reject) {
                  if (!order_cancel_reject.text.empty())
                    ++processed;
                },
                [](const fix::PositionReport& position_report) {
                },
                [](const fix::Reject& reject) {
                },
                [](const fix::ResendRequest& resend_request) {
                },
                [](const fix::SecurityList& security_list) {
                },
                [](const fix::TestRequest& test_request) {
                },
                [](const fix::UserResponse& user_response) {
                },
              },
              message,
              buffer);
        },
        MESSAGE,
        std::strlen(MESSAGE));
  }
}

BENCHMARK(BM_fix_parser_dispatch_order_cancel_reject);
