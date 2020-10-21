/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/debug.h"
#include "roq/core/fix/reader.h"

#include "roq/deribit/fix/new_order_single.h"

using namespace roq;           // NOLINT
using namespace roq::deribit;  // NOLINT

TEST(fix_new_order_single, create_message) {
  core::utils::Buffer buffer(4096);
  auto msg_seq_num = uint64_t{0};
  auto sending_time = std::chrono::seconds{1568702810};
  fix::NewOrderSingle new_order_single = {
      .cl_ord_id = "roq-ord-006",
      .side = core::fix::Side::BUY,
      .order_qty = 2.0,
      .price = 0.5,
      .symbol = "BTC-27SEP19",
      .exec_inst = std::string_view(),
      .ord_type = core::fix::OrdType::LIMIT,
      .time_in_force = core::fix::TimeInForce::GTC,
      .deribit_label = "roq;123;345",
      .deribit_adv_order_type = '\0',
  };
  core::fix::Writer writer(
      buffer,
      core::fix::Version::FIX_44,
      decltype(new_order_single)::msg_type,
      "ROQ_TRADING",
      "DERIBITSERVER",
      msg_seq_num,
      sending_time);
  auto message = new_order_single.encode(writer);
  // core::print_string_with_escapes(message.data(), message.length());
  constexpr auto expected =
      "8=FIX.4.4\0019=0000159\00135=D\00149=ROQ_TRADING\00156=DERIBIT"
      "SERVER\00134=1\00152=20190917-06:46:50.000\00111=roq-ord-006\001"
      "54=1\00138=2.00000000\00144=0.50000000\00155=BTC-27SEP19\00140"
      "=2\00159=1\001100010=roq;123;345\00110=154\001";
  ASSERT_EQ(message.length(), std::strlen(expected));
  for (size_t i = 0; i < message.length(); ++i)
    EXPECT_EQ(static_cast<char>(message.data()[i]), expected[i]);
}
