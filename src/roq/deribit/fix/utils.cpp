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

}  // namespace fix
}  // namespace deribit
}  // namespace roq
