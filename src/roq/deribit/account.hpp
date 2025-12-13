/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <utility>

#include "roq/deribit/config.hpp"

#include "roq/deribit/tools/crypto.hpp"

namespace roq {
namespace deribit {

struct Account final {
  Account(Config const &, std::string_view const &name);

  Account(Account const &) = delete;

  std::string create_nonce();

  std::pair<std::string, std::chrono::milliseconds> create_signature(std::chrono::milliseconds timestamp, std::string_view const &nonce) {
    return crypto_.create_signature(timestamp, nonce);
  }

  std::string create_raw_data(std::chrono::milliseconds timestamp) { return crypto_.create_raw_data(timestamp); }

  std::string create_password(std::string_view const &raw_data);

  std::string const name;
  std::string const key;

 private:
  tools::Crypto crypto_;
};

}  // namespace deribit
}  // namespace roq
