/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/flags/args.hpp"

#include "roq/deribit/pcap_dump/flags/flags.hpp"

namespace roq {
namespace deribit {
namespace pcap_dump {

struct Settings final : public flags::Flags {
  explicit Settings(args::Parser const &);
};

}  // namespace pcap_dump
}  // namespace deribit
}  // namespace roq
