/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/api/enums.h"

#include "roq/api.h"

namespace roq {
namespace deribit {
namespace api {

template <>
api::State to_enum(const std::string_view& value) {
  if (ROQ_UNLIKELY(value.empty()))
    return api::State::UNKNOWN;
  switch (value.data()[0]) {
    case 'o': {
      if (value.compare("open") == 0) {
        return api::State::OPEN;
      }
      break;
    }
  }
  return api::State::UNKNOWN;
}

}  // namespace api
}  // namespace deribit
}  // namespace roq
