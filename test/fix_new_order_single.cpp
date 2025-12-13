/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include <fmt/format.h>

#include "roq/utils/debug/fix/message.hpp"

#include "roq/fix/reader.hpp"

#include "roq/deribit/fix/new_order_single.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

using NewOrderSingle = deribit::fix::NewOrderSingle;

TEST_CASE("fix_new_order_single_create_message", "[fix_new_order_single]") {
  std::vector<std::byte> buffer(4096);
  auto msg_seq_num = uint64_t{0};
  auto sending_time = 1568702810s;
  auto new_order_single = NewOrderSingle{
      .cl_ord_id = "roq-ord-006"sv,
      .side = roq::fix::Side::BUY,
      .order_qty = {2.0, Precision::_1},
      .price = {0.45, Precision::_2},
      .symbol = "BTC-27SEP19"sv,
      .exec_inst = {},
      .ord_type = roq::fix::OrdType::LIMIT,
      .time_in_force = roq::fix::TimeInForce::GTC,
      .stop_px = {},
      .deribit_label = "roq;123;345"sv,
      .deribit_adv_order_type = '\0',
      .deribit_mm_protection = {},
      .deribit_condition_trigger_method = {},
  };
  auto header = roq::fix::Header{
      .version = roq::fix::Version::FIX_44,
      .msg_type = decltype(new_order_single)::MSG_TYPE,
      .sender_comp_id = "ROQ_TRADING"sv,
      .target_comp_id = "DERIBITSERVER"sv,
      .msg_seq_num = ++msg_seq_num,  // note!
      .sending_time = sending_time,
  };
  auto message = new_order_single.encode(header, buffer);
  auto const expected =
      "8=FIX.4.4\001"
      "9=0000146\001"
      "35=D\001"
      "49=ROQ_TRADING\001"
      "56=DERIBITSERVER\001"
      "34=1\001"
      "52=20190917-06:46:50.000\001"
      "11=roq-ord-006\001"
      "54=1\001"
      "38=2.0\001"
      "44=0.45\001"
      "55=BTC-27SEP19\001"
      "40=2\001"
      "59=1\001"
      "100010=roq;123;345\001"
      "10=042\001"sv;
  fmt::print("{}\n"sv, utils::debug::fix::Message{message});
  fmt::print("{}\n"sv, utils::debug::fix::Message{expected});
  REQUIRE(std::size(message) == std::size(expected));
  for (size_t i = 0; i < std::size(message); ++i) {
    CHECK(static_cast<char>(std::data(message)[i]) == expected[i]);
  }
}
