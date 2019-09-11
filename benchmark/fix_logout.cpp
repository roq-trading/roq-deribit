/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/patterns.h"

#include "roq/deribit/fix/parser.h"

using namespace roq;  // NOLINT
using namespace roq::deribit;  // NOLINT

namespace {
static const char *MESSAGE =
  "8=FIX.4.4\0019=90\00135=5\00149=DERIBITSERVER\00156=ROQ_TRADIN"
  "G\00134=1\00152=20190907-16:56:43.398\00158=invalid_credential"
  "s\00110=166\001";
}  // namespace

void BM_fix_logout_parse_message(benchmark::State& state) {
  uint64_t processed = 0;
  for (auto _ : state) {
    auto bytes = core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
        [&](const core::fix::message_t& message) {
          auto logout = fix::Logout::parse(message);
          if (logout.text > 0)
            ++processed;
        },
        MESSAGE,
        std::strlen(MESSAGE));
  }
}

BENCHMARK(BM_fix_logout_parse_message);

void BM_fix_parser_dispatch_logout(benchmark::State& state) {
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
                [&](const fix::Logout& logout) {
                  if (logout.text.length() > 0)
                    ++processed;
                },
                [](const fix::MarketDataIncrementalRefresh& market_data_incremental_refresh) {
                },
                [](const fix::MarketDataRequestReject& market_data_request_reject) {
                },
                [](const fix::MarketDataSnapshotFullRefresh& market_data_snapshot_full_refresh) {
                },
                [](const fix::OrderCancelReject& order_cancel_reject) {
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

BENCHMARK(BM_fix_parser_dispatch_logout);
