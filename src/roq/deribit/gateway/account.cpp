/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/gateway/account.hpp"

#include "roq/logging.hpp"

#include "roq/utils/safe_cast.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {
namespace gateway {

// === HELPERS ===

namespace {
auto create_key(auto &config, auto &name) {
  auto key = config.get_access_key(name);
  if (std::empty(key)) {
    log::fatal(R"(Unexpected: missing key for name="{}")"sv, name);
  }
  return key;
}

auto create_crypto(auto &config, auto &name) {
  auto secret = config.get_access_secret(name);
  if (std::empty(secret)) {
    log::fatal(R"(Unexpected: missing secret for name="{}")"sv, name);
  }
  return tools::Crypto{secret};
}
}  // namespace

// === IMPLEMENTATION ===

Account::Account(Config const &config, std::string_view const &name) : name{name}, key{create_key(config, name)}, crypto_{create_crypto(config, name)} {
}

std::pair<std::string, std::chrono::milliseconds> Account::create_signature(TraceInfo const &trace_info, std::string_view const &nonce) {
  return crypto_.create_signature(utils::safe_cast(trace_info.origin_create_time_utc), nonce);
}

std::string Account::create_raw_data(TraceInfo const &trace_info) {
  return crypto_.create_raw_data(utils::safe_cast(trace_info.origin_create_time_utc));
}

}  // namespace gateway
}  // namespace deribit
}  // namespace roq
