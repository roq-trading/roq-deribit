/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/deribit/gateway/settings.hpp"

namespace roq {
namespace deribit {
namespace gateway {

struct API final {
  struct {
    std::string_view currencies;
    std::string_view instruments;
    std::string_view chart_data;
  } market_data;
  struct {
  } order_management;

  // factory
  static API create(Settings const &);
};

}  // namespace gateway
}  // namespace deribit
}  // namespace roq
