/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/websocket.h"

#include <fmt/format.h>
#include <fmt/chrono.h>

#include "roq/patterns.h"

#include "roq/core/clock.h"

#include "roq/core/ws/decoder.h"
#include "roq/core/ws/encoder.h"
#include "roq/core/ws/random.h"
#include "roq/core/ws/upgrade.h"

#include "roq/core/http/response.h"

namespace roq {
namespace deribit {

namespace {
constexpr auto PING_FREQUENCY = std::chrono::seconds{10};
constexpr auto DECODE_BUFFER_SIZE = size_t{1048576};  // FIXME(thraneh): flag
}  // namespace

WebSocket::WebSocket(
    Controller& controller,
    core::ssl::Context& ssl_context,
    core::event::Base& base,
    core::event::DNSBase& dns_base,
    const core::URI& uri)
    : _controller(controller),
      _ssl_connection(ssl_context),
      _dns_base(dns_base),
      _uri(uri),
      _timer(base, EV_PERSIST, [this]() { on_timer(); }),
      _buffer_event(base, _ssl_connection),
      _response(*this),
      _decode_buffer(DECODE_BUFFER_SIZE) {
  LOG_IF(FATAL, _uri.scheme.compare("wss") != 0) <<
    "Expected URI scheme to be \"wss\" (got \"" << _uri.scheme << "\")";
  int value = 1;
  setsockopt(_buffer_event.getfd(), IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));
  _timer.add(std::chrono::seconds{1});
  _buffer_event.setcb(
      [this]() { on_read(); },
      [this](int events) { on_error(events); });
  _buffer_event.enable(EV_READ);
}

void WebSocket::start() {
  VLOG(1) << "connect("
    "host=\"" << _uri.host << "\", "
    "port=" << _uri.get_port_with_default() <<
    ")";
  _buffer_event.connect(
      _dns_base,
      AF_INET,
      _uri.host,
      _uri.get_port_with_default());
}

void WebSocket::send(const std::string_view& message) {
  VLOG(4) << "send(length=" << message.length() << ")";
  _buffer_event.write(message.data(), message.length());
  _buffer_event.flush(EV_WRITE, BEV_FLUSH);
}

// bufferevent:

void WebSocket::on_read() {
  _buffer_event.read(_buffer);
  switch (_state) {
    case State::UPGRADE:
      throw std::runtime_error("Unexpected [request]");
      break;
    case State::UPGRADE_SENT:
      process_upgrade_response();
      if (_state != State::UPGRADED)
        break;
      [[ fallthrough ]];
    case State::UPGRADED:
      process_ws_data();
      break;
    default:
      throw std::runtime_error("Unexpected");
  }
}

void WebSocket::on_error(int events) {
  if (events & BEV_EVENT_CONNECTED) {
    send_upgrade_request();
  } else {
    _controller.on_ws_disconnect();
  }
}

void WebSocket::on_timer() {
  auto now = core::get_time();
  if (now < _next_update)
    return;
  _next_update = now + PING_FREQUENCY;
  switch (_state) {
    case State::UPGRADED:
      send_ping();
      break;
    default:
      break;
  }
}

// http:

void WebSocket::send_upgrade_request() {
  assert(_state == State::UPGRADE);
  assert(_response_key.empty());
  auto key = core::ws::Random::create_sec_websocket_key();
  _response_key = core::ws::Random::create_response(key);
  char message[4096];
  core::ws::Writer writer(message, std::size(message));
  core::ws::Upgrade::create(writer, _uri, key);
  send(writer);
  _state = State::UPGRADE_SENT;
}

// FIXME(thraneh): what about redirect?
void WebSocket::process_upgrade_response() {
  assert(_state == State::UPGRADE_SENT);
  while (true) {
    auto length = _buffer.length();
    if (length == 0)
      return;
    auto buffer = _buffer.pullup(length);
    auto bytes = _response.execute(
        reinterpret_cast<const char *>(buffer),
        length);
    if (bytes == 0)  // waiting for something
      return;
    _buffer.drain(bytes);
    if (_state == State::UPGRADED) {
      _controller.on_ws_ready();
      return;
    }
  }
}

// ws:

void WebSocket::process_ws_data() {
  assert(_state == State::UPGRADED);
  while (true) {
    auto length = _buffer.length();
    if (length == 0)
      return;
    auto buffer = _buffer.pullup(length);
    auto bytes = core::ws::Decoder::dispatch(
        overloaded {
          [](const core::ws::continuation_t&) {
            LOG(WARNING) << "Unexpected [continuation]";
          },
          [this](const core::ws::text_t& text) {
            process(text);
          },
          [](const core::ws::binary_t&) {
            LOG(WARNING) << "Unexpected [binary]";
          },
          [this](const core::ws::close_t& close) {
            process(close);
          },
          [this](const core::ws::ping_t& ping) {
            process(ping);
          },
          [this](const core::ws::pong_t& pong) {
            process(pong);
          },
        },
        buffer,
        length);
    if (bytes == 0)  // waiting for something
      return;
    _buffer.drain(bytes);
  }
}

void WebSocket::process(const core::ws::text_t& text) {
  if (ROQ_UNLIKELY(text.last == false)) {
    LOG(WARNING) << "Unexpected [fragmented]";
    return;
  }
  /*
  json::Parser::dispatch(
      _controller,
      text.payload,
      _decode_buffer);
  */
}

void WebSocket::process(const core::ws::close_t& close) {
  LOG(WARNING) << "close reason=" << close.reason;
}

void WebSocket::process(const core::ws::ping_t& ping) {
  VLOG(1) << "ping(length=" << ping.length << ")";
  char message[4096];
  core::ws::Writer writer(message, std::size(message));
  core::ws::Encoder::pong(writer, ping.payload, ping.length);
  send(writer);
}

void WebSocket::process(const core::ws::pong_t& pong) {
  if (pong.length >= sizeof(std::chrono::nanoseconds)) {
    auto& send_time = *reinterpret_cast<
      const std::chrono::nanoseconds *>(pong.payload);
    auto latency = core::get_time() - send_time;
    VLOG(1) << fmt::format("pong(send_time={}, latency={})",
        send_time,
        std::chrono::duration_cast<std::chrono::milliseconds>(latency));
  } else {
    VLOG(1) << "pong(...)";
  }
}

void WebSocket::send_close() {
  char message[4096];
  core::ws::Writer writer(message, std::size(message));
  core::ws::Encoder::close(writer, 1000);
  send(writer);
  // FIXME(thraneh): it is mandated to shutdown our end of the connection
}

void WebSocket::send_ping() {
  std::chrono::nanoseconds now = core::get_time();
  char message[4096];
  core::ws::Writer writer(message, std::size(message));
  core::ws::Encoder::ping(writer, &now, sizeof(now));
  send(writer);
}

// http:

void WebSocket::on_message_begin() {
  assert(_status == core::http::Status::UNKNOWN);
  assert(_header == core::http::Header::UNKNOWN);
}

void WebSocket::on_url(const char *, size_t) {
  assert(false);  // only client
}

void WebSocket::on_status(int code, const char *, size_t) {
  assert(_header == core::http::Header::UNKNOWN);
  _status = core::http::parse_status(code);
  if (_status == core::http::Status::SWITCHING_PROTOCOLS) {
    LOG(INFO) << fmt::format("status={} ({})", code, _status);
  } else {
    throw std::runtime_error(
        fmt::format(
          "Expected status code 101 (Switching Protocols),"
          "got status code {} ({})",
          code, _status));
  }
}

void WebSocket::on_header_field(const char *data, size_t length) {
  std::string_view value(data, length);
  _header = core::http::parse_header(value);
}

void WebSocket::on_header_value(const char *data, size_t length) {
  std::string_view value(data, length);
  switch (_header) {
    case core::http::Header::CONNECTION: {
      LOG(INFO) << fmt::format("{}=\"{}\"", _header, value);
      if (value.compare("upgrade") == 0) {
        _connection_upgrade = true;
      } else {
        LOG(WARNING) << fmt::format("Expected \"upgrade\", got \"{}\"", value);
      }
      break;
    }
    case core::http::Header::UPGRADE: {
      LOG(INFO) << fmt::format("{}=\"{}\"", _header, value);
      if (value.compare("websocket") == 0) {
        _upgrade_websocket = true;
      } else {
        LOG(WARNING) << fmt::format("Expected \"websocket\", got \"{}\"", value);
      }
      break;
    }
    case core::http::Header::SEC_WEBSOCKET_ACCEPT: {
      LOG(INFO) << fmt::format("{}=\"{}\"", _header, value);
      if (value.compare(_response_key) == 0) {
        _sec_websocket_accept = true;
      } else {
        LOG(WARNING) << fmt::format("Expected \"websocket\", got \"{}\"", value);
      }
      break;
    }
    default: {
    }
  }
  _header = core::http::Header::UNKNOWN;
}

void WebSocket::on_headers_complete() {
  assert(_header == core::http::Header::UNKNOWN);
}

void WebSocket::on_chunk_header() {
  assert(_header == core::http::Header::UNKNOWN);
  LOG(WARNING) << "Unexpected [chunk header]";
}

void WebSocket::on_body(const char *, size_t) {
  assert(_header == core::http::Header::UNKNOWN);
  LOG(WARNING) << "Unexpected [body]";
}

void WebSocket::on_chunk_complete() {
  assert(_header == core::http::Header::UNKNOWN);
  LOG(WARNING) << "Unexpected [chunk complete]";
}

void WebSocket::on_message_complete() {
  assert(_header == core::http::Header::UNKNOWN);
  _status = core::http::Status::UNKNOWN;
  if (_connection_upgrade && _upgrade_websocket && _sec_websocket_accept) {
    _state = State::UPGRADED;
    LOG(INFO) << "Connection has now been upgraded to websocket";
  } else {
    throw std::runtime_error("Connection has not been correctly upgraded to websocket");
  }
}

}  // namespace deribit
}  // namespace roq
