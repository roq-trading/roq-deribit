/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/deribit/authenticator.hpp"

namespace roq {
namespace deribit {

// === IMPLEMENTATION ===

Authenticator::Authenticator(Config const &config, std::string_view const &account)
    : account_{account}, key_{config.get_access_key(account)}, crypto_{config.get_access_secret(account)} {
}

std::string Authenticator::create_nonce() {
  return crypto_.create_nonce();
}

std::string Authenticator::create_password(std::string_view const &raw_data) {
  return crypto_.create_password(raw_data);
}

}  // namespace deribit
}  // namespace roq
