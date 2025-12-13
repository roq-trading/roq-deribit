/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/deribit/account.hpp"

#include "roq/logging.hpp"

using namespace std::literals;

namespace roq {
namespace deribit {

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

std::string Account::create_nonce() {
  return crypto_.create_nonce();
}

std::string Account::create_password(std::string_view const &raw_data) {
  return crypto_.create_password(raw_data);
}

}  // namespace deribit
}  // namespace roq
