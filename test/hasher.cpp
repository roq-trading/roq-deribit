/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/deribit/tools/hasher.hpp"

using namespace roq;
using namespace roq::deribit;

using namespace std::literals;
using namespace std::chrono_literals;

using namespace Catch::literals;

TEST_CASE("hasher_test_1", "[hasher]") {  // DERIBIT'S EXAMPLE
  tools::Hasher hasher("94Qy1rwus9zxRKrvnSEbb1YCcKZunhI7g9ZHlhGUoQM"sv);
  auto timestamp = 1637681707400ms;
  auto nonce = "sfx7kglb6r2outb74dnut65vlywu4csr"sv;
  auto [signature, used_timestamp] = hasher.create_signature(timestamp, nonce);
  CHECK(signature == "64064fb648aaa12eb60e87ed3410b18039bca746a670e684783389a1cd374e93"sv);
}

TEST_CASE("hasher_test_2", "[hasher]") {  // VERIFY ON DEV BOX
  tools::Hasher hasher("94Qy1rwus9zxRKrvnSEbb1YCcKZunhI7g9ZHlhGUoQM"sv);
  auto timestamp = 1637688869368ms;
  auto nonce = "ha4sge1mygftldeh0yaw6v4zl3q5acif"sv;
  auto [signature, used_timestamp] = hasher.create_signature(timestamp, nonce);
  CHECK(signature == "3c733e856ca5eaa74d13d70e18ef7668482f9bdebe56ac0a0c78987e0f217d07"sv);
}
/*
TEST_CASE("hasher_test_3", "[hasher]") {  // RENCAP'S EXAMPLE
  tools::Hasher hasher("94Qy1rwus9zxRKrvnSEbb1YCcKZunhI7g9ZHlhGUoQM"sv);
  auto timestamp = 1637688598837ms;
  auto nonce = "a5kl5fel7cviqck80856lajoc13ahio8"sv;
  auto signature = hasher.create_signature(timestamp, nonce);
  CHECK(signature == "b16b1fd43d5a71ffbc18092e757ed07ec173a8f1022a32e4c19dbb8101ccb0f9"sv);
}

TEST_CASE("hasher_test_4", "[hasher]") {  // RENCAP'S LAST EXAMPLE
  tools::Hasher hasher("94Qy1rwus9zxRKrvnSEbb1YCcKZunhI7g9ZHlhGUoQM"sv);
  auto timestamp = 1637691257449ms;
  auto nonce = "ql32qq5m1medc8ewbhbtzglszh8z3hqg"sv;
  auto signature = hasher.create_signature(timestamp, nonce);
  CHECK(signature == "5db820ea54c3340074574b0e0b2924aaf7e149af2af55b9e865170a1219021bb"sv);
}
*/
