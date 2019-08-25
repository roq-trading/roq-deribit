/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/controller.h"

#include "roq/deribit/gateway.h"

namespace roq {
namespace deribit {

namespace {
constexpr auto DECODE_BUFFER_SIZE = size_t{1048576};  // FIXME(thraneh): flag
}  // namespace

Controller::Controller(Gateway& gateway)
    : _gateway(gateway),
      _decode_buffer(DECODE_BUFFER_SIZE) {
}

// rest api:

void Controller::on_rest_connected() {
}

void Controller::on_rest_disconnected() {
}

// websocket api:

void Controller::on_ws_ready() {
  /*
  _gateway.websocket().send_subscribe_common();
  _gateway.rest().enqueue(
      "/products",
      [this](const std::string_view& body) {
        (*this)(json::Products::parse_message(body, _decode_buffer));
      },
      []() {
        LOG(FATAL) << "Failed to get products";
      });
      */
}

void Controller::on_ws_disconnect() {
}

}  // namespace deribit
}  // namespace roq
