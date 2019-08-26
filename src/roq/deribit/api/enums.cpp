/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/api/enums.h"

#include "roq/api.h"

namespace roq {
namespace deribit {
namespace api {

template <>
api::Kind to_enum(const std::string_view& value) {
  if (ROQ_UNLIKELY(value.empty()))
    return api::Kind::UNKNOWN;
  switch (value.data()[0]) {
    case 'f':
      if (value.compare("future") == 0)
        return api::Kind::FUTURE;
      break;
    case 'o':
      if (value.compare("option") == 0)
        return api::Kind::OPTION;
      break;
  }
  return api::Kind::UNKNOWN;
}

template <>
api::OptionType to_enum(const std::string_view& value) {
  if (ROQ_UNLIKELY(value.empty()))
    return api::OptionType::UNKNOWN;
  switch (value.data()[0]) {
    case 'c':
      if (value.compare("call") == 0)
        return api::OptionType::CALL;
      break;
    case 'p':
      if (value.compare("put") == 0)
        return api::OptionType::PUT;
      break;
  }
  return api::OptionType::UNKNOWN;
}

template <>
api::State to_enum(const std::string_view& value) {
  if (ROQ_UNLIKELY(value.empty()))
    return api::State::UNKNOWN;
  switch (value.data()[0]) {
    case 'c':
      if (value.compare("closed") == 0)
        return api::State::CLOSED;
      break;
    case 'o':
      if (value.compare("open") == 0)
        return api::State::OPEN;
      break;
  }
  return api::State::UNKNOWN;
}

}  // namespace api
}  // namespace deribit
}  // namespace roq
