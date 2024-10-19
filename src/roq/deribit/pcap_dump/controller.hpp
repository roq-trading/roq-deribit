/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/deribit/pcap_dump/settings.hpp"

namespace roq {
namespace deribit {
namespace pcap_dump {

struct Controller final {
  Controller(Settings const &, std::string_view const &pcap_path);

  Controller(Controller const &) = delete;
  Controller(Controller &&) = delete;

  void dispatch();

 private:
  Settings const &settings_;
  std::string const pcap_path_;
};

}  // namespace pcap_dump
}  // namespace deribit
}  // namespace roq
