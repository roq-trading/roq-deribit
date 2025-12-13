/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/deribit/json/instrument_state.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;
/*
TEST_CASE("subscribe_ack", "[json_instrument_state]") {
  auto message = R"({)"
                 R"("jsonrpc":"2.0",)"
                 R"("id":"subscribe_instrument_state",)"
                 R"("result":[)"
                 R"("instrument.state.any.any")"
                 R"(],)"
                 R"("usIn":1765425743121014,)"
                 R"("usOut":1765425743121136,)"
                 R"("usDiff":122,)"
                 R"("testnet":false)"
                 R"(})"sv;
}
*/
TEST_CASE("simple", "[json_instrument_state]") {
  auto message = R"({)"
                 R"(})"sv;
}
