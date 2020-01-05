/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <vector>

#include "roq/core/uri.h"
#include "roq/core/ssl/ssl.h"

#include "roq/core/event/base.h"
#include "roq/core/event/buffer.h"
#include "roq/core/event/buffer_event.h"
#include "roq/core/event/dns_base.h"

#include "roq/core/utils/message.h"

namespace roq {
namespace deribit {

class Gateway;

class FIX final {
 public:
  FIX(
      Gateway& gateway,
      core::ssl::Context& ssl_context,
      core::event::Base& base,
      core::event::DNSBase& dns_base,
      const core::URI& uri,
      size_t decode_buffer_size);

  void start();
  void stop();

  void send(const core::utils::Message& message);

 private:
  void on_read();
  void on_error(int err);

  void process_data();

 private:
  Gateway& _gateway;
  core::ssl::Context _ssl_context;
  core::ssl::Connection _ssl_connection;
  core::event::DNSBase& _dns_base;
  const core::URI _uri;
  core::event::BufferEvent _buffer_event;
  core::event::Buffer _buffer;
  core::utils::Buffer _decode_buffer;
};

}  // namespace deribit
}  // namespace roq
