/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <chrono>

namespace roq {
namespace deribit {
namespace gateway {

struct Request final {
  // currencies
  std::chrono::nanoseconds request_currencies = {};
  std::chrono::nanoseconds respond_currencies = {};
  // instruments
  std::chrono::nanoseconds request_instruments = {};
  std::chrono::nanoseconds respond_instruments = {};
};

}  // namespace gateway
}  // namespace deribit
}  // namespace roq
