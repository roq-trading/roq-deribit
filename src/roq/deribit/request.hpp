/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include <chrono>

namespace roq {
namespace deribit {

struct Request final {
  std::chrono::nanoseconds request_instruments = {};
  std::chrono::nanoseconds respond_instruments = {};
};

}  // namespace deribit
}  // namespace roq
