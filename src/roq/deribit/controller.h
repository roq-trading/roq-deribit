/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <vector>

#include "roq/logging.h"

namespace roq {
namespace deribit {

class Gateway;

class Controller final {
 public:
  explicit Controller(Gateway& gateway);

  Controller(Controller&) = delete;
  void operator=(Controller&) = delete;

  // rest api:
  void on_rest_connected();
  void on_rest_disconnected();

  // websocket api:
  void on_ws_ready();
  void on_ws_disconnect();

  // fix api:
  // TODO(thraneh): connect/disconnect

 private:
  Gateway& _gateway;
  std::vector<std::byte> _decode_buffer;
};

}  // namespace deribit
}  // namespace roq
