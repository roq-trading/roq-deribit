/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/core/fix/reader.h"
// #include "roq/core/debug.h"

#include "roq/deribit/fix/order_cancel_request.h"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

TEST(fix_order_cancel_request, create_message) {
  core::Buffer buffer(4096);
  auto msg_seq_num = uint64_t{0};
  auto sending_time = std::chrono::seconds{1568702810};
  fix::OrderCancelRequest order_cancel_request = {
      .cl_ord_id = "123"sv,
      .orig_cl_ord_id = "123"sv,
  };
  core::fix::Writer writer(
      buffer,
      core::fix::Version::FIX_44,
      decltype(order_cancel_request)::msg_type,
      "ROQ_TRADING"sv,
      "DERIBITSERVER"sv,
      msg_seq_num,
      sending_time);
  auto message = order_cancel_request.encode(writer);
  // core::print_string_with_escapes(message.data(), message.length());
  const auto expected =
      "8=FIX.4.4\0019=0000081\00135=F\00149=ROQ_TRADING\00156=DERIBIT"
      "SERVER\00134=1\00152=20190917-06:46:50.000\00111=123\00141=123"
      "\00110=128\001"sv;
  ASSERT_EQ(std::size(message), std::size(expected));
  for (size_t i = 0; i < std::size(message); ++i)
    EXPECT_EQ(static_cast<char>(message.data()[i]), expected[i]);
}
