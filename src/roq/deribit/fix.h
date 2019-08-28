/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <vector>

#include "roq/core/uri.h"
#include "roq/core/ssl/ssl.h"
#include "roq/core/event/event.h"
#include "roq/core/http/response.h"
#include "roq/core/ws/decoder.h"

#include "roq/deribit/controller.h"

namespace roq {
namespace deribit {

class FIX final {
 public:
  FIX(
      Controller& controller,
      core::ssl::Context& ssl_context,
      core::event::Base& base,
      core::event::DNSBase& dns_base,
      const core::URI& uri);

  void start();
  void send(const std::string_view& message);

 private:
  void on_read();
  void on_error(int err);

  void on_timer();

  void process_data();

 private:
  Controller& _controller;
  core::ssl::Connection _ssl_connection;
  core::event::DNSBase& _dns_base;
  const core::URI _uri;
  core::event::Timer _timer;
  core::event::BufferEvent _buffer_event;
  core::event::Buffer _buffer;
  std::vector<std::byte> _decode_buffer;
};

}  // namespace deribit
}  // namespace roq
