/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/fix/common.hpp"

namespace roq {
namespace deribit {
namespace gateway {

constexpr auto FIX_VERSION = fix::Version::FIX_44;

constexpr std::string_view SENDER_COMP_ID = "ROQ_TRADING";
constexpr std::string_view TARGET_COMP_ID = "DERIBITSERVER";

}  // namespace gateway
}  // namespace deribit
}  // namespace roq
