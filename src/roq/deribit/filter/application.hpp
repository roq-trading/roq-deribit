/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/tool.hpp"

namespace roq {
namespace deribit {
namespace filter {

struct Application final : public roq::Tool {
  using roq::Tool::Tool;

 protected:
  int main(args::Parser const &) override;
};

}  // namespace filter
}  // namespace deribit
}  // namespace roq
