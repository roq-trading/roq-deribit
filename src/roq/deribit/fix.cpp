/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix.h"

#include "roq/core/debug.h"

#include "roq/deribit/gateway.h"

#include "roq/deribit/fix/parser.h"

namespace roq {
namespace deribit {

namespace {
constexpr auto DECODE_BUFFER_SIZE = size_t{1048576};  // FIXME(thraneh): flag
}  // namespace

FIX::FIX(
    Gateway& gateway,
    core::ssl::Context& ssl_context,
    core::event::Base& base,
    core::event::DNSBase& dns_base,
    const core::URI& uri)
    : _gateway(gateway),
      _ssl_connection(ssl_context),
      _dns_base(dns_base),
      _uri(uri),
      _buffer_event(base),  //, _ssl_connection),
      _decode_buffer(DECODE_BUFFER_SIZE) {
  LOG_IF(FATAL, _uri.scheme.compare("tcp") != 0) <<
    "Expected URI scheme to be \"tcp\" (got \"" << _uri.scheme << "\")";
  int value = 1;
  setsockopt(_buffer_event.getfd(), IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));
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

void FIX::send(const core::utils::Message& message) {
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
    _gateway.on_fix_connected();
  } else {
    _gateway.on_fix_disconnected();
  }
}

// fix:

void FIX::process_data() {
  for (;;) {
    auto length = _buffer.length();
    if (length == 0)
      return;
    auto buffer = _buffer.pullup(length);
    auto bytes = core::fix::Reader<fix::FIX_VERSION>::dispatch(
        [&](const core::fix::message_t& message) {
          try {
            fix::Parser::dispatch(
                [&](const auto& event) {
                  _gateway(event, message.header.msg_seq_num);
                },
                message,
                _decode_buffer);
          } catch (std::exception& e) {
            fprintf(stderr, "*** ERROR *** %s\n", e.what());
            core::print_memory(buffer, length);
            core::print_string_with_escapes(buffer, length);
            throw;
          }
        },
        buffer,
        length);
    if (bytes == 0)
      return;
    // core::print_string_with_escapes(buffer, bytes);
    _buffer.drain(bytes);
  }
}

}  // namespace deribit
}  // namespace roq
