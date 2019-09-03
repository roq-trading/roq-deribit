/* Copyright (c) 2017-2019, Hans Erik Thrane */

#pragma once

#include <cstdint>

namespace roq {
namespace deribit {
namespace fix {

enum class Deribit : uint32_t {
  INSTRUMENT_PRICE_PRECISION = 2576,
  CONDITION_TRIGGER_METHOD = 5127,
  CANCEL_ON_DISCONNECT = 9001,
  USE_WORDSAFE_TAGS = 9002,
  TRADE_AMOUNT = 100007,
  SINCE_TIMESTAMP = 100008,
  TRADE_ID = 100009,
  LABEL = 100010,
  ADV_ORDER_TYPE = 100012,
  TRADE_VOLUME_24H = 100087,
  MARK_PRICE = 100090,
};

}  // namespace fix
}  // namespace deribit
}  // namespace roq
