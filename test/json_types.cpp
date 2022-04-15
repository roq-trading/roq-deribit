/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include <catch2/catch.hpp>

#include "roq/deribit/json/utils.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;

using namespace Catch::literals;

TEST_CASE("json_double", "[json_types]") {
  {
    double result = 1.0;
    core::json::value_t value = "undefined"sv;
    json::update(result, value);
    CHECK(std::isnan(result) == true);
  }
  {
    double result = 1.0;
    core::json::value_t value = core::json::null_t{};
    json::update(result, value);
    CHECK(std::isnan(result) == true);
  }
  {
    double result = NaN;
    core::json::value_t value = 1.2;
    json::update(result, value);
    CHECK(result == 1.2);
  }
}
