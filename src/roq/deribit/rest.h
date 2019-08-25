/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <list>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "roq/core/uri.h"
#include "roq/core/ssl/ssl.h"
#include "roq/core/event/event.h"
#include "roq/core/http/request.h"
#include "roq/core/http/response.h"
#include "roq/core/ws/decoder.h"

namespace roq {
namespace deribit {

using success_t = std::function<void(const std::string_view&)>;
using failure_t = std::function<void()>;

class HTTPConnection final : public core::http::Response::Handler {
 public:
  struct connected_t final {
  };
  struct disconnected_t final {
  };
  struct status_t final {
    core::http::Status status;
  };
  struct header_t final {
    core::http::Header header;
    std::string_view value;
  };
  struct body_t final {
    const void *data;
    size_t length;
  };
  struct Handler {
    virtual void operator()(const connected_t&) = 0;
    virtual void operator()(const disconnected_t&) = 0;
    virtual void operator()(const status_t&) = 0;
    virtual void operator()(const header_t&) = 0;
    virtual void operator()(const body_t&) = 0;
  };
  HTTPConnection(
      Handler& handler,
      core::ssl::Context& ssl_context,
      core::event::Base& base)
      : _handler(handler),
        _ssl_connection(ssl_context),
        _buffer_event(base, _ssl_connection),
        _response(*this),
        _chunk_buffer(1024*1024) {
    // FIXME(thraneh): setsockopt -> core
    int value = 1;
    setsockopt(_buffer_event.getfd(), IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));
    _buffer_event.setcb(
        [this]() { on_read(); },
        [this](int events) { on_error(events); });
    _buffer_event.enable(EV_READ);
  }

  HTTPConnection(HTTPConnection&) = delete;
  void operator=(HTTPConnection&) = delete;

  void connect(
      core::event::DNSBase& dns_base,
      const core::URI& uri) {
    assert(_state == State::DISCONNECTED);
    _buffer_event.connect(
        dns_base,
        AF_INET,
        uri.host,
        uri.get_port_with_default());
    _state = State::CONNECTING;
  }

  void write(const void *data, size_t length) {
    _buffer_event.write(data, length);
  }

 protected:
  void on_read() {
    _buffer_event.read(_buffer);
    if (_buffer.empty())
      return;
    auto bytes = _buffer.length();
    auto buffer = _buffer.pullup(bytes);
    auto length = _response.execute(
        reinterpret_cast<const char *>(buffer),
        bytes);
    _buffer.drain(length);
  }
  void on_error(int events) {
    if (events & BEV_EVENT_CONNECTED) {
      _handler(connected_t{});
    } else {
      _handler(disconnected_t{});
    }
  }

 protected:
  void on_message_begin() override {
    assert(_status == core::http::Status::UNKNOWN);
    assert(_header == core::http::Header::UNKNOWN);
    assert(_buffer_offset == 0);
  }
  void on_url(const char *, size_t) override {
    assert(false);  // only client
  }
  void on_status(int code, const char *, size_t) override {
    assert(_header == core::http::Header::UNKNOWN);
    assert(_buffer_offset == 0);
    _status = core::http::parse_status(code);
    _handler(status_t{.status = _status});
  }
  void on_header_field(const char *data, size_t length) override {
    assert(_buffer_offset == 0);
    std::string_view value(data, length);
    _header = core::http::parse_header(value);
  }
  void on_header_value(const char *data, size_t length) override {
    assert(_buffer_offset == 0);
    std::string_view value(data, length);
    _handler(header_t{.header = _header, .value = value});
    _header = core::http::Header::UNKNOWN;
  }
  void on_headers_complete() override {
    assert(_header == core::http::Header::UNKNOWN);
    assert(_buffer_offset == 0);
  }
  void on_chunk_header() override {
    assert(_header == core::http::Header::UNKNOWN);
  }
  void on_body(const char *data, size_t length) override {
    assert(_header == core::http::Header::UNKNOWN);
    std::memcpy(
        _chunk_buffer.data() + _buffer_offset,
        data,
        length);
    _buffer_offset += length;
  }
  void on_chunk_complete() override {
    assert(_header == core::http::Header::UNKNOWN);
  }
  void on_message_complete() override {
    assert(_header == core::http::Header::UNKNOWN);
    _handler(body_t{.data = _chunk_buffer.data(), .length = _buffer_offset});
    _status = core::http::Status::UNKNOWN;
    _buffer_offset = 0;
  }

 private:
  Handler& _handler;
  core::ssl::Connection _ssl_connection;
  core::event::BufferEvent _buffer_event;
  core::event::Buffer _buffer;
  enum State {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
  } _state = State::DISCONNECTED;
  // http-parser
  core::http::Response _response;
  core::http::Status _status = core::http::Status::UNKNOWN;
  core::http::Header _header = core::http::Header::UNKNOWN;
  std::vector<std::byte> _chunk_buffer;
  size_t _buffer_offset = 0;
};


class Rest final : public HTTPConnection::Handler {
 public:
  Rest(
      core::ssl::Context& ssl_context,
      core::event::Base& base,
      core::event::DNSBase& dns_base,
      const core::URI& uri);

  Rest(Rest&) = delete;
  void operator=(Rest&) = delete;

  void enqueue(
      std::string&& uri,
      success_t&& success,
      failure_t&& failure);

 private:
  void on_timer();

  void check_timeout();

  void connect();

  void process_pending();

  bool request(
      const core::http::Method& method,
      const std::string_view& uri);

  bool throttle();

  void make_pending(
      std::string&& uri,
      success_t&& success,
      failure_t&& failure);

  void make_sent(
      success_t&& success,
      failure_t&& failure);

  bool remove_one(
      const core::http::Status& status,
      const std::string_view& body);
  void remove_all();

 protected:
  void operator()(const HTTPConnection::connected_t&);
  void operator()(const HTTPConnection::disconnected_t&);
  void operator()(const HTTPConnection::status_t&);
  void operator()(const HTTPConnection::header_t&);
  void operator()(const HTTPConnection::body_t&);

 private:
  core::ssl::Context& _ssl_context;
  core::event::Base& _base;
  core::event::DNSBase& _dns_base;
  const core::URI _uri;
  core::event::Timer _timer;
  std::unique_ptr<HTTPConnection> _connection;
  enum State {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    DISCONNECTING,
  } _state = State::DISCONNECTED;
  // request pipeline
  std::list<std::tuple<std::string, success_t, failure_t> > _waiting;
  std::list<std::tuple<success_t, failure_t> > _sent;
  // throttling
  std::chrono::nanoseconds _window = {};
  int _request_count = 0;
  // weird to have here ...
  core::http::Status _status = core::http::Status::UNKNOWN;
};

}  // namespace deribit
}  // namespace roq
