/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/flags/args.hpp"

#include "roq/deribit/filter/flags/flags.hpp"
#include "roq/deribit/filter/flags/multicast.hpp"

namespace roq {
namespace deribit {
namespace filter {

struct Settings final : public flags::Flags {
  explicit Settings(args::Parser const &);

  flags::multicast multicast;
};

}  // namespace filter
}  // namespace deribit
}  // namespace roq
