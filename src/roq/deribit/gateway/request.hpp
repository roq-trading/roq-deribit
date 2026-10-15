/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/core/limit/request.hpp"

#include "roq/deribit/gateway/shared.hpp"

namespace roq {
namespace deribit {
namespace gateway {

struct Request final : public core::limit::Request::Handler {
  explicit Request(Shared &);

  // core::Request::Handler

  bool ready() const override;
  bool can_request(TraceInfo const &) override;

  // ...

  core::limit::Request currencies;
  core::limit::Request instruments;

 private:
  Shared &shared_;
};

}  // namespace gateway
}  // namespace deribit
}  // namespace roq
