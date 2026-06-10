/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/deribit/protocol/json/utils.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

TEST_CASE("json_double", "[json_types]") {
  {
    double result = 1.0;
    core::json::Value value = "undefined"sv;
    protocol::json::update(result, value);
    CHECK(std::isnan(result) == true);
  }
  {
    double result = 1.0;
    core::json::Value value = core::json::Null{};
    protocol::json::update(result, value);
    CHECK(std::isnan(result) == true);
  }
  {
    double result = NaN;
    core::json::Value value = 1.2;
    protocol::json::update(result, value);
    CHECK(result == 1.2_a);
  }
}
