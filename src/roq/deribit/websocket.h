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

class WebSocket final : public core::http::Response::Handler {
 public:
  WebSocket(
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

  // http:
  void send_upgrade_request();
  void process_upgrade_response();

  // websocket:
  void process_ws_data();
  void process(const core::ws::text_t&);
  void process(const core::ws::close_t&);
  void process(const core::ws::ping_t&);
  void process(const core::ws::pong_t&);

  void send_close();
  void send_ping();

 protected:
  void on_message_begin() override;
  void on_url(const char *, size_t) override;
  void on_status(int, const char *, size_t) override;
  void on_header_field(const char *, size_t) override;
  void on_header_value(const char *, size_t) override;
  void on_headers_complete() override;
  void on_chunk_header() override;
  void on_body(const char *, size_t) override;
  void on_chunk_complete() override;
  void on_message_complete() override;

 private:
  Controller& _controller;
  core::ssl::Connection _ssl_connection;
  core::event::DNSBase& _dns_base;
  const core::URI _uri;
  core::event::Timer _timer;
  core::event::BufferEvent _buffer_event;
  core::event::Buffer _buffer;
  core::http::Response _response;
  std::vector<std::byte> _decode_buffer;
  enum class State {
    UPGRADE,
    UPGRADE_SENT,
    UPGRADED
  } _state = State::UPGRADE;
  std::string _response_key;
  std::chrono::nanoseconds _next_update = {};
  // ...
  core::http::Status _status = core::http::Status::UNKNOWN;
  core::http::Header _header = core::http::Header::UNKNOWN;
  // ...
  bool _connection_upgrade = false;
  bool _upgrade_websocket = false;
  bool _sec_websocket_accept = false;
};

}  // namespace deribit
}  // namespace roq
