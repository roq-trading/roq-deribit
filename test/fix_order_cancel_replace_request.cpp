/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/fix/reader.h"
// #include "roq/core/debug.h"

#include "roq/deribit/fix/order_cancel_replace_request.h"

using namespace roq;  // NOLINT
using namespace roq::deribit;  // NOLINT

TEST(fix_order_cancel_replace_request, create_message) {
  core::utils::Buffer buffer(4096);
  auto msg_seq_num = uint64_t{0};
  auto sending_time = std::chrono::seconds{1568702810};
  auto message = fix::OrderCancelReplaceRequest::encode(
      buffer,
      msg_seq_num,
      sending_time,
      "123",
      "123",
      core::fix::Side::BUY,
      1.0,
      core::fix::OrdType::LIMIT,
      1.0,
      "BTC-27SEP19",
      sending_time);
  // core::print_string_with_escapes(message.data(), message.length());
  constexpr auto expected =
    "8=FIX.4.4\0019=0000155\00135=G\00149=ROQ_TRADING\00156=DERIBIT"
    "SERVER\00134=1\00152=20190917-06:46:50.000\00111=123\00141=123"
    "\00160=20190917-06:46:50.000\00154=1\00138=1.000000\00140=2\001"
    "44=1.000000\00155=BTC-27SEP19\00110=186\001";
  ASSERT_EQ(message.length(), std::strlen(expected));
  for (size_t i = 0; i < message.length(); ++i)
    EXPECT_EQ(
        static_cast<char>(message.data()[i]),
        expected[i]);
}
