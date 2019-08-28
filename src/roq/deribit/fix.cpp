/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix.h"

#include <fmt/format.h>
#include <fmt/chrono.h>

#include "roq/patterns.h"

#include "roq/core/clock.h"

#include "roq/core/fix/reader.h"
#include "roq/core/fix/writer.h"

namespace roq {
namespace deribit {

namespace {
constexpr auto PING_FREQUENCY = std::chrono::seconds{10};
constexpr auto DECODE_BUFFER_SIZE = size_t{1048576};  // FIXME(thraneh): flag
}  // namespace

FIX::FIX(
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
      _buffer_event(base),  //, _ssl_connection),
      _decode_buffer(DECODE_BUFFER_SIZE) {
  LOG_IF(FATAL, _uri.scheme.compare("tcp") != 0) <<
    "Expected URI scheme to be \"tcp\" (got \"" << _uri.scheme << "\")";
  int value = 1;
  setsockopt(_buffer_event.getfd(), IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));
  _timer.add(std::chrono::seconds{1});
  _buffer_event.setcb(
      [this]() { on_read(); },
      [this](int events) { on_error(events); });
  _buffer_event.enable(EV_READ);
}

void FIX::start() {
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

void FIX::send(const std::string_view& message) {
  VLOG(4) << "send(length=" << message.length() << ")";
  _buffer_event.write(message.data(), message.length());
  _buffer_event.flush(EV_WRITE, BEV_FLUSH);
}

// bufferevent:

void FIX::on_read() {
  _buffer_event.read(_buffer);
  process_data();
}

void FIX::on_error(int events) {
  if (events & BEV_EVENT_CONNECTED) {
    LOG(INFO) << "CONNECTED";
    char buffer[4096];
    core::fix::Writer writer(buffer, std::size(buffer));
    auto message = writer
      .write(core::fix::Field::MSG_TYPE, 'A')
      .write(core::fix::Field::SENDER_COMP_ID, "ROQ_TRADING")
      .write(core::fix::Field::TARGET_COMP_ID, "DERIBIT_SERVER")
      .write(core::fix::Field::MSG_SEQ_NUM, uint64_t{1})
      .write(core::fix::Field::SENDING_TIME, core::get_time())
      .finish();
    _buffer_event.write(message.data(), message.length());
  } else {
    _controller.on_ws_disconnect();
  }
}

void FIX::on_timer() {
  /*
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
  */
}

// fix:

void FIX::process_data() {
  auto length = _buffer.length();
  if (length == 0)
    return;
  auto buffer = _buffer.pullup(length);
  auto bytes = core::fix::Reader::dispatch(
      [](const core::fix::value_t& value) {
        LOG(INFO) << "field=" << value.field << ", value=" << value.value;
      },
      buffer,
      length);
  if (bytes > 0)
    _buffer.drain(bytes);
}

}  // namespace deribit
}  // namespace roq
