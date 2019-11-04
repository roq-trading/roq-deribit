/* Copyright (c) 2017-2019, Hans Erik Thrane */

#include "roq/deribit/fix/utils.h"

namespace roq {
namespace deribit {
namespace fix {

SecurityType map_security_type(const std::string_view& value) {
  if (value.length() == 3) {
    switch (value.data()[0]) {
      case 'F':
        if (value.compare("FUT") == 0)
          return SecurityType::FUTURES;
        break;
      case 'O':
        if (value.compare("OPT") == 0)
          return SecurityType::OPTION;
        break;
    }
  }
  return SecurityType::UNDEFINED;
}

Error map_error(const std::string_view& value) {
  if (value.length() > 0) {
    switch (value.data()[0]) {
      case 'c':
        if (value.compare("canceled") == 0)
          return Error::NONE;
        break;
      case 's':
        if (value.compare("success") == 0)
          return Error::NONE;
        break;
    }
  }
  return Error::UNKNOWN;
}

}  // namespace fix
}  // namespace deribit
}  // namespace roq
