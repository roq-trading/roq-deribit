/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/fix/reader.hpp"

#include "roq/deribit/fix/reject.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

using Reject = deribit::fix::Reject;

TEST_CASE("fix_reject_parse_message", "[fix_reject]") {
  auto const message =
      "8=FIX.4.4\0019=98\00135=3\00149=DERIBITSERVER\00156=ROQ_TRADIN"
      "G\00134=5\00152=20190908-08:47:31.543\00145=5\001372=AN\00158="
      "not_implemented\00110=092\001"sv;
  int results = 0;
  auto parser = [&](auto &message_2) {
    ++results;
    CHECK(message_2.header.msg_type == roq::fix::MsgType::REJECT);
    auto reject = Reject::create(message_2);
    CHECK(reject.ref_seq_num == uint64_t{5});
    CHECK(reject.ref_msg_type == roq::fix::MsgType::REQUEST_FOR_POSITIONS);
    CHECK(reject.text == "not_implemented"sv);
  };
  auto bytes = roq::fix::Reader<roq::fix::Version::FIX_44>::dispatch(message, parser);
  CHECK(bytes == std::size(message));
  CHECK(results == 1);
}
