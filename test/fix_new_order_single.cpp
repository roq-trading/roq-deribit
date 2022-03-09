/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <catch2/catch.hpp>

#include "roq/core/debug.hpp"
#include "roq/core/fix/reader.hpp"

#include "roq/deribit/fix/new_order_single.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("fix_new_order_single_create_message", "[fix_new_order_single]") {
  core::Buffer buffer(4096);
  auto msg_seq_num = uint64_t{0};
  auto sending_time = 1568702810s;
  fix::NewOrderSingle new_order_single = {
      .cl_ord_id = "roq-ord-006"sv,
      .side = core::fix::Side::BUY,
      .order_qty = {2.0, utils::to_decimals(1)},
      .price = {0.45, utils::to_decimals(2)},
      .symbol = "BTC-27SEP19"sv,
      .exec_inst = {},
      .ord_type = core::fix::OrdType::LIMIT,
      .time_in_force = core::fix::TimeInForce::GTC,
      .deribit_label = "roq;123;345"sv,
      .deribit_adv_order_type = '\0',
  };
  core::fix::Writer writer(
      buffer,
      core::fix::Version::FIX_44,
      decltype(new_order_single)::msg_type,
      "ROQ_TRADING"sv,
      "DERIBITSERVER"sv,
      msg_seq_num,
      sending_time);
  auto message = new_order_single.encode(writer);
  // core::print_string_with_escapes(message);
  const auto expected =
      "8=FIX.4.4\0019=0000146\00135=D\00149=ROQ_TRADING\00156=DERIBIT"
      "SERVER\00134=1\00152=20190917-06:46:50.000\00111=roq-ord-006\001"
      "54=1\00138=2.0\00144=0.45\00155=BTC-27SEP19\00140=2\00159=1\001"
      "100010=roq;123;345\00110=042\001"sv;
  REQUIRE(std::size(message) == std::size(expected));
  for (size_t i = 0; i < std::size(message); ++i)
    CHECK(static_cast<char>(std::data(message)[i]) == expected[i]);
}
