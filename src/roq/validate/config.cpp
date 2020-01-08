/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/validate/config.h"

#include "roq/validate/options.h"

namespace roq {
namespace deribit {
namespace validate {

void Config::dispatch(Handler& handler) const {
  // accounts
  handler(
      client::Account {
        .regex = FLAGS_account,
      });
  // symbols
  handler(
      client::Symbol {
        .regex = FLAGS_symbol,
        .exchange = FLAGS_exchange,
      });
  // currencies
  handler(
      client::Symbol {
        .regex = FLAGS_currencies,
      });
}

}  // namespace validate
}  // namespace deribit
}  // namespace roq
