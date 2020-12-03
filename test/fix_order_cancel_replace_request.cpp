/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/debug.h"
#include "roq/core/fix/reader.h"

#include "roq/deribit/fix/order_cancel_replace_request.h"

using namespace roq;           // NOLINT
using namespace roq::deribit;  // NOLINT

TEST(fix_order_cancel_replace_request, create_message) {
  core::utils::Buffer buffer(4096);
  auto msg_seq_num = uint64_t{0};
  auto sending_time = std::chrono::seconds{1568702810};
  fix::OrderCancelReplaceRequest order_cancel_replace_request = {
      .orig_cl_ord_id = "123",
      .cl_ord_id = "123",
      .transact_time = sending_time,
      .side = core::fix::Side::BUY,
      .order_qty = 1.0,
      .ord_type = core::fix::OrdType::LIMIT,
      .price = 1.0,
      .symbol = "BTC-27SEP19",
      .exec_inst = {},
  };
  core::fix::Writer writer(
      buffer,
      core::fix::Version::FIX_44,
      decltype(order_cancel_replace_request)::msg_type,
      "ROQ_TRADING",
      "DERIBITSERVER",
      msg_seq_num,
      sending_time);
  auto message = order_cancel_replace_request.encode(writer);
  // core::print_string_with_escapes(message.data(), message.length());
  constexpr auto expected =
      "8=FIX.4.4\0019=0000159\00135=G\00149=ROQ_TRADING\00156=DERIBIT"
      "SERVER\00134=1\00152=20190917-06:46:50.000\00141=123\00111=123"
      "\00160=20190917-06:46:50.000\00154=1\00138=1.00000000\00140=2\001"
      "44=1.00000000\00155=BTC-27SEP19\00110=126\001";
  ASSERT_EQ(message.length(), std::strlen(expected));
  for (size_t i = 0; i < message.length(); ++i)
    EXPECT_EQ(static_cast<char>(message.data()[i]), expected[i]);
}
