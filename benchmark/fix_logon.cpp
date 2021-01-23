/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/deribit/fix/logon.h"

using namespace roq;           // NOLINT
using namespace roq::deribit;  // NOLINT

namespace {
static const char *MESSAGE =
    "8=FIX.4.4\0019=211\00135=A\00149=DERIBITSERVER\00156=ROQ_TRADI"
    "NG\00134=1\00152=20190907-16:45:58.192\001108=10\00195=58\0019"
    "6=1567874758168.y4/hA3i6qxm4yVL+3N7IrGcINVAFMLFhy4l7ATSehxc=\001"
    "553=5MP40u9h\001554=j/tVe9IsQuc+RjegscnHcJ6czMVNM1+ib7vjbY3UV0"
    "M=\0019001=Y\00110=115\001";
}  // namespace

// cppcheck-suppress constParameterCallback
void BM_fix_logon_parse_message(benchmark::State &state) {
  uint64_t processed = 0;
  for (auto _ : state) {
    core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
        [&](const core::fix::message_t &message) {
          auto result = fix::Logon::create(message);
          if (result.heart_bt_int > 0)
            ++processed;
        },
        MESSAGE,
        std::strlen(MESSAGE));
  }
}

BENCHMARK(BM_fix_logon_parse_message);

// cppcheck-suppress constParameterCallback
void BM_fix_logon_create_message(benchmark::State &state) {
  core::utils::Buffer buffer(4096);
  auto msg_seq_num = uint64_t{0};
  auto sending_time = std::chrono::seconds{1568702810};
  std::string_view raw_data =
      "1567874758168.y4/hA3i6qxm4yVL+3N7IrGcINVAFMLFhy4l7ATSehxc=";
  uint64_t processed = 0;
  for (auto _ : state) {
    fix::Logon logon = {
        .heart_bt_int = uint16_t{10},
        .raw_data_length = static_cast<uint32_t>(raw_data.size()),
        .raw_data = raw_data.data(),
        .username = "5MP40u9h",
        .password = "j/tVe9IsQuc+RjegscnHcJ6czMVNM1+ib7vjbY3UV0M=",
        .use_wordsafe_tags = false,
        .cancel_on_disconnect = true,
        .deribit_app_id = {},
        .deribit_app_sig = {},
    };
    core::fix::Writer writer(
        buffer,
        core::fix::Version::FIX_44,
        decltype(logon)::msg_type,
        "ROQ_TRADING",
        "DERIBITSERVER",
        msg_seq_num,
        sending_time);
    auto message = logon.encode(writer);
    if (message.length())
      ++processed;
  }
}

BENCHMARK(BM_fix_logon_create_message);
