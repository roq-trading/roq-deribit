/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <catch2/catch.hpp>

#include "roq/core/fix/reader.h"

#include "roq/deribit/fix/reject.h"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

TEST_CASE("fix_reject_parse_message", "fix_reject") {
  const auto message =
      "8=FIX.4.4\0019=98\00135=3\00149=DERIBITSERVER\00156=ROQ_TRADIN"
      "G\00134=5\00152=20190908-08:47:31.543\00145=5\001372=AN\00158="
      "not_implemented\00110=092\001"sv;
  int results = 0;
  auto bytes = core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
      [&](const core::fix::message_t &message) {
        ++results;
        CHECK(message.header.msg_type == core::fix::MsgType::REJECT);
        auto reject = fix::Reject::create(message);
        CHECK(reject.ref_seq_num == uint64_t{5});
        CHECK(reject.ref_msg_type == core::fix::MsgType::REQUEST_FOR_POSITIONS);
        CHECK(reject.text == "not_implemented"sv);
      },
      message);
  CHECK(bytes == std::size(message));
  CHECK(results == 1);
}
