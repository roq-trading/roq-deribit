/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/fix/reader.hpp"

#include "roq/deribit/fix/reject.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

TEST_CASE("fix_reject_parse_message", "[fix_reject]") {
  auto const message =
      "8=FIX.4.4\0019=98\00135=3\00149=DERIBITSERVER\00156=ROQ_TRADIN"
      "G\00134=5\00152=20190908-08:47:31.543\00145=5\001372=AN\00158="
      "not_implemented\00110=092\001"sv;
  int results = 0;
  auto bytes = core::fix::Reader<core::fix::Version::FIX_44>::dispatch(
      [&](core::fix::Message const &message_2) {
        ++results;
        CHECK(message_2.header.msg_type == core::fix::MsgType::REJECT);
        auto reject = fix::Reject::create(message_2);
        CHECK(reject.ref_seq_num == uint64_t{5});
        CHECK(reject.ref_msg_type == core::fix::MsgType::REQUEST_FOR_POSITIONS);
        CHECK(reject.text == "not_implemented"sv);
      },
      message);
  CHECK(bytes == std::size(message));
  CHECK(results == 1);
}
