/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/fix/reader.hpp"

#include "roq/deribit/fix/logon.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;
using namespace std::chrono_literals;

using Logon = deribit::fix::Logon;

namespace {
auto const MESSAGE =
    "8=FIX.4.4\0019=211\00135=A\00149=DERIBITSERVER\00156=ROQ_TRADI"
    "NG\00134=1\00152=20190907-16:45:58.192\001108=10\00195=58\0019"
    "6=1567874758168.y4/hA3i6qxm4yVL+3N7IrGcINVAFMLFhy4l7ATSehxc=\001"
    "553=5MP40u9h\001554=j/tVe9IsQuc+RjegscnHcJ6czMVNM1+ib7vjbY3UV0"
    "M=\0019001=Y\00110=115\001"sv;
}  // namespace

void BM_fix_logon_parse_message(benchmark::State &state) {
  auto parser = [&](auto &message_2) { Logon::create(message_2); };
  for (auto _ : state) {
    roq::fix::Reader<roq::fix::Version::FIX_44>::dispatch(MESSAGE, parser);
  }
}

BENCHMARK(BM_fix_logon_parse_message);

void BM_fix_logon_create_message(benchmark::State &state) {
  std::vector<std::byte> buffer(4096);
  uint64_t msg_seq_num = 0;
  auto sending_time = 1568702810s;
  auto raw_data = "1567874758168.y4/hA3i6qxm4yVL+3N7IrGcINVAFMLFhy4l7ATSehxc="sv;
  for (auto _ : state) {
    auto logon = Logon{
        .heart_bt_int = uint16_t{10},
        .raw_data_length = static_cast<uint32_t>(std::size(raw_data)),
        .raw_data = std::data(raw_data),
        .username = "5MP40u9h"sv,
        .password = "j/tVe9IsQuc+RjegscnHcJ6czMVNM1+ib7vjbY3UV0M="sv,
        .use_wordsafe_tags = false,
        .cancel_on_disconnect = true,
        .deribit_app_id = {},
        .deribit_app_sig = {},
    };
    auto header = roq::fix::Header{
        .version = roq::fix::Version::FIX_44,
        .msg_type = decltype(logon)::MSG_TYPE,
        .sender_comp_id = "ROQ_TRADING"sv,
        .target_comp_id = "DERIBITSERVER"sv,
        .msg_seq_num = ++msg_seq_num,  // note!
        .sending_time = sending_time,
    };
    logon.encode(header, buffer);
  }
}

BENCHMARK(BM_fix_logon_create_message);
