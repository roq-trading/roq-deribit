/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <span>
#include <string>

namespace roq {
namespace deribit {
namespace gateway {

struct CurrenciesUpdate final {
  std::span<std::string const> currencies;
};

}  // namespace gateway
}  // namespace deribit
}  // namespace roq
