/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/fix/reader.hpp"

#include "roq/deribit/fix/order_cancel_request.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("fix_order_cancel_request_create_message", "[fix_order_cancel_request]") {
  std::vector<std::byte> buffer(4096);
  auto msg_seq_num = uint64_t{0};
  auto sending_time = 1568702810s;
  auto order_cancel_request = fix::OrderCancelRequest{
      .cl_ord_id = "123"sv,
      .orig_cl_ord_id = "123"sv,
  };
  auto header = core::fix::Header{
      .version = core::fix::Version::FIX_44,
      .msg_type = decltype(order_cancel_request)::msg_type,
      .sender_comp_id = "ROQ_TRADING"sv,
      .target_comp_id = "DERIBITSERVER"sv,
      .msg_seq_num = ++msg_seq_num,  // note!
      .sending_time = sending_time,
  };
  auto message = order_cancel_request.encode(header, buffer);
  auto const expected =
      "8=FIX.4.4\0019=0000081\00135=F\00149=ROQ_TRADING\00156=DERIBIT"
      "SERVER\00134=1\00152=20190917-06:46:50.000\00111=123\00141=123"
      "\00110=128\001"sv;
  REQUIRE(std::size(message) == std::size(expected));
  for (size_t i = 0; i < std::size(message); ++i)
    CHECK(static_cast<char>(std::data(message)[i]) == expected[i]);
}
