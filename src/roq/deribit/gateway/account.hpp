/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <utility>

#include "roq/trace_info.hpp"

#include "roq/deribit/gateway/config.hpp"

#include "roq/deribit/tools/crypto.hpp"

namespace roq {
namespace deribit {
namespace gateway {

struct Account final {
  Account(Config const &, std::string_view const &name);

  Account(Account const &) = delete;

  std::string create_nonce() { return crypto_.create_nonce(); }

  std::pair<std::string, std::chrono::milliseconds> create_signature(TraceInfo const &, std::string_view const &nonce);

  std::string create_raw_data(TraceInfo const &);

  std::string create_password(std::string_view const &raw_data) { return crypto_.create_password(raw_data); }

  std::string const name;
  std::string const key;

 private:
  tools::Crypto crypto_;
};

}  // namespace gateway
}  // namespace deribit
}  // namespace roq
